#include "text_merge_plan.h"
#include "prepared_text_pool.h"
#include "gmd_text_allocator.h"
#include "text_prepare_transaction.h"
#include "gmd_snapshot.h"
#include "text_snapshot_transaction.h"
#include "safe_memory_read.h"
#include "gmd_storage_capture.h"
#include "gmd_prepare_pipeline.h"
#include "text_stream_origin.h"
#include "native_file_origin.h"
#include "mount_origin_snapshot.h"
#include "opened_source_snapshot.h"
#include "stormbolt_stage_core.h"
#include "generated_stormbolt_rows.h"
#include "stormbolt_shot_core.h"
#include "stormbolt_coefficient_adapter.h"
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
static std::size_t fail_new_countdown = 0;
static bool throw_unexpected = false;
void* operator new(std::size_t size) {
    if (fail_new_countdown && --fail_new_countdown == 0) {
        if (throw_unexpected) throw std::runtime_error("injected allocation exception");
        throw std::bad_alloc();
    }
    if (auto* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
struct AllocState { std::size_t calls = 0, fail_at = 0, live = 0; };
void* allocate_test(void* context, std::size_t size) noexcept {
    auto& state = *static_cast<AllocState*>(context);
    if (++state.calls == state.fail_at) return nullptr;
    auto* result = std::malloc(size);
    if (result) ++state.live;
    return result;
}
void free_test(void* context, void* pointer) noexcept {
    --static_cast<AllocState*>(context)->live;
    std::free(pointer);
}
void* allocate_gmd_test(void* context, std::uint32_t size) noexcept {
    return allocate_test(context, size);
}
struct CopyFault { std::size_t calls = 0, fail_at = 0; bool stale = false, name_change = false; };
struct GmdFixture {
    std::vector<unsigned char> header, records, key_pool, text_pool, pointers;
    explicit GmdFixture(const stormslinger::OwnedTextSnapshot& snapshot) : header(0x918),
        records(snapshot.keys.size() * 32), pointers(snapshot.strings.size() * 8) {
        std::vector<std::size_t> key_offsets, text_offsets;
        for (const auto& key : snapshot.keys) {
            key_offsets.push_back(key_pool.size());
            key_pool.insert(key_pool.end(), key.key.begin(), key.key.end()); key_pool.push_back(0);
        }
        for (const auto& text : snapshot.strings) {
            text_offsets.push_back(text_pool.size());
            text_pool.insert(text_pool.end(), text.begin(), text.end()); text_pool.push_back(0);
        }
        for (std::size_t n = 0; n < snapshot.keys.size(); ++n) {
            const auto index = static_cast<std::uint32_t>(snapshot.keys[n].index);
            const auto address = reinterpret_cast<std::uintptr_t>(key_pool.data() + key_offsets[n]);
            std::memcpy(records.data() + n * 32, &index, 4);
            std::memcpy(records.data() + n * 32 + 16, &address, 8);
        }
        for (std::size_t n = 0; n < snapshot.strings.size(); ++n) {
            const auto address = reinterpret_cast<std::uintptr_t>(text_pool.data() + text_offsets[n]);
            std::memcpy(pointers.data() + n * 8, &address, 8);
        }
        const auto put32 = [&](std::size_t offset, std::size_t value) {
            const auto word = static_cast<std::uint32_t>(value); std::memcpy(header.data() + offset, &word, 4);
        };
        const auto put64 = [&](std::size_t offset, std::uintptr_t value) { std::memcpy(header.data() + offset, &value, 8); };
        put64(0, 0x123456); put32(0xA8, 0x10302);
        put32(0xB8, snapshot.keys.size()); put32(0xD8, snapshot.strings.size());
        put32(0xC8, key_pool.size()); put32(0xE8, text_pool.size());
        put64(0xC0, reinterpret_cast<std::uintptr_t>(records.data()));
        put64(0xD0, reinterpret_cast<std::uintptr_t>(key_pool.data()));
        put64(0xF0, reinterpret_cast<std::uintptr_t>(text_pool.data()));
        put64(0xF8, reinterpret_cast<std::uintptr_t>(pointers.data()));
    }
    GmdFixture(const GmdFixture&) = delete;
    GmdFixture& operator=(const GmdFixture&) = delete;
};
stormslinger::MemoryReadResult copy_fault(void* context, std::uintptr_t address, std::size_t bytes,
    std::size_t maximum, std::vector<unsigned char>& output) noexcept {
    auto& fault = *static_cast<CopyFault*>(context);
    if (++fault.calls == fault.fail_at) return stormslinger::MemoryReadResult::unreadable;
    const auto result = stormslinger::copy_current_process_memory(address, bytes, maximum, output);
    if (result == stormslinger::MemoryReadResult::copied && fault.stale && fault.calls == 6)
        output[0x64] = 1;
    if (result == stormslinger::MemoryReadResult::copied && fault.name_change && fault.calls == 6)
        output[0x0C] = 'X';
    return result;
}
int main() {
    {
        stormslinger::StormboltStageState state{};
        const std::array<std::uint32_t, 9> expected{94,94,94,95,95,96,96,96,97};
        for (std::uint32_t i = 0; i < 9; ++i) {
            state.shot_snapshot = i;
            assert(stormslinger::select_stormbolt_resource(state) == expected[i]);
            assert(state.shot_snapshot == i);
        }
        state.shot_snapshot = UINT32_MAX;
        assert(stormslinger::select_stormbolt_resource(state) == 97);
        assert(state.shot_snapshot == UINT32_MAX);
        auto custom = expected;
        custom[3] = 123;
        state.shot_snapshot = 3;
        assert(stormslinger::select_stormbolt_resource(state, custom) == 123);
    }
    using namespace stormslinger;
    {
        std::array<unsigned char, 0x6B0> buffer{};
        buffer.fill(0xA5);
        const auto untouched = buffer;
        StormboltStageState state{};
        state.shot_snapshot = UINT32_MAX;
        for (const auto id : {0u, 237u, 240u}) {
            assert(apply_stormbolt_coefficients(13, id, 40, true, buffer.data(), buffer.size(),
                state, ps_stormbolt_rows) == CoefficientApplyResult::unrelated);
            assert(buffer == untouched && state.shot_snapshot == UINT32_MAX);
        }
        for (std::uint32_t category = 0; category < 15; ++category) {
            if (category == 13) continue;
            for (const auto id : {238u, 239u}) {
                assert(apply_stormbolt_coefficients(category, id, 40, true,
                    buffer.data(), buffer.size(), state, ps_stormbolt_rows)
                    == CoefficientApplyResult::unrelated);
                assert(buffer == untouched && state.shot_snapshot == UINT32_MAX);
            }
        }
        assert(apply_stormbolt_coefficients(13, 239, 39, true, buffer.data(), buffer.size(),
            state, ps_stormbolt_rows) == CoefficientApplyResult::unrelated);
        for (const auto type_confirmed : {false, true}) {
            const auto bytes = type_confirmed ? buffer.size()-1 : buffer.size();
            assert(apply_stormbolt_coefficients(13, 239, 40, type_confirmed, buffer.data(), bytes,
                state, ps_stormbolt_rows) == CoefficientApplyResult::invalid_target);
        }
        assert(apply_stormbolt_coefficients(13, 239, 40, true, nullptr, buffer.size(),
            state, ps_stormbolt_rows) == CoefficientApplyResult::invalid_target);
        auto invalid_rows = ps_stormbolt_rows;
        invalid_rows[8].fields[6] = std::numeric_limits<float>::quiet_NaN();
        assert(apply_stormbolt_coefficients(13, 239, 40, true, buffer.data(), buffer.size(),
            state, invalid_rows) == CoefficientApplyResult::invalid_parameters);
        assert(buffer == untouched && state.shot_snapshot == UINT32_MAX);
        for (const auto id : {238u, 239u}) {
            for (std::uint32_t snapshot = 0; snapshot < 10; ++snapshot) {
                buffer = untouched;
                state.shot_snapshot = snapshot == 9 ? UINT32_MAX : snapshot;
                const auto row = (std::min)(snapshot, 8u);
                assert(apply_stormbolt_coefficients(13, id, 40, true, buffer.data(), buffer.size(),
                    state, ps_stormbolt_rows) == CoefficientApplyResult::applied);
                assert(state.shot_snapshot == row);
                auto expected = untouched;
                std::memcpy(expected.data()+0x60, &ps_stormbolt_rows[row].fields[5], sizeof(float));
                std::memcpy(expected.data()+0x6C, &ps_stormbolt_rows[row].fields[6], sizeof(float));
                assert(buffer == expected); // All other fields, including native references, preserved.
            }
        }
    }
    {
        StormboltStageState state;
        StormboltCoefficients output;
        for (std::uint32_t row = 0; row < 9; ++row) {
            state.shot_snapshot = row;
            assert(prepare_stormbolt_coefficients(40, state, ps_stormbolt_rows, output));
            assert(output.context60 == ps_stormbolt_rows[row].fields[5]);
            assert(output.context6c == ps_stormbolt_rows[row].fields[6]);
            float gauge = 5;
            assert(subtract_stormbolt_energy(gauge, state, ps_stormbolt_rows) && gauge == 3);
            for (float amount : {2.0f, 1.0f, -1.0f}) {
                gauge = amount;
                assert(subtract_stormbolt_energy(gauge, state, ps_stormbolt_rows) && gauge == 0);
            }
        }
        state.shot_snapshot = 100;
        const auto retained = output;
        assert(!prepare_stormbolt_coefficients(39, state, ps_stormbolt_rows, output));
        assert(state.shot_snapshot == 100 && output.context6c == retained.context6c);
        float gauge = 5;
        assert(!subtract_stormbolt_energy(gauge, state, ps_stormbolt_rows) && gauge == 5);
        assert(prepare_stormbolt_coefficients(40, state, ps_stormbolt_rows, output));
        assert(state.shot_snapshot == 8);
    }
    {
        std::ofstream exported("../research/compiled-stormbolt-rows.bin", std::ios::binary);
        assert(exported);
        StormboltStageTiming timing;
        for (std::size_t row = 0; row < ps_stormbolt_rows.size(); ++row) {
            const auto& data = ps_stormbolt_rows[row];
            timing.action_row[row] = data.fields[0];
            timing.decay_row[row] = data.fields[1];
            assert(data.fields[2] == 2.0f);
            for (const auto value : data.fields)
                exported.write(reinterpret_cast<const char*>(&value), sizeof(value));
            exported.write(reinterpret_cast<const char*>(&data.flag), 1);
        }
        assert(exported.good());
        timing.post_shot_decay = ps_post_shot_decay_default;
        StormboltStageState state;
        for (std::uint32_t shot = 0; shot < 9; ++shot) {
            assert(advance_stormbolt_stage(state, timing));
            if (shot < 8) assert(state.action_timer == ps_stormbolt_rows[shot + 1].fields[0]);
        }
        assert(decay_stormbolt_stage(state, timing, 2));
        assert(state.counter == 8 && state.decay_timer == ps_stormbolt_rows[8].fields[1]);
    }
    {
        StormboltStageTiming timing;
        for (std::size_t row = 0; row < 9; ++row) {
            timing.action_row[row] = static_cast<float>(row + 10);
            timing.decay_row[row] = static_cast<float>(row + 20);
        }
        timing.post_shot_decay = 5;
        StormboltStageState state;
        for (std::uint32_t shot = 0; shot < 9; ++shot) {
            assert(advance_stormbolt_stage(state, timing));
            assert(state.shot_snapshot == shot && state.counter == shot + 1);
            assert(stormbolt_shot_row(state) == shot && state.decay_timer == 5);
            if (shot < 8) assert(state.action_timer == timing.action_row[shot + 1]);
        }
        const auto held = state.action_timer;
        assert(advance_stormbolt_stage(state, timing));
        assert(state.counter == 10 && stormbolt_shot_row(state) == 8 && state.action_timer == held);
        assert(decay_stormbolt_stage(state, timing, 100));
        assert(state.counter == 8 && state.decay_timer == 28); // One step, fresh row timer.
        assert(decay_stormbolt_stage(state, timing, 1));
        assert(state.counter == 8 && state.decay_timer == 27);
        state.counter = 1; state.decay_timer = 0;
        assert(decay_stormbolt_stage(state, timing, 1));
        assert(state.counter == 0 && state.decay_timer == 20);
        assert(decay_stormbolt_stage(state, timing, 100));
        assert(state.counter == 0 && state.decay_timer == 20);
    }
    {
        std::vector<unsigned char> mount(0x238), record(0x148);
        mount[4] = 1; mount[8] = 1;
        std::memcpy(mount.data() + 0x0C, "nativePC", 9);
        record[0] = 1; record[0x2C] = 2;
        std::memcpy(record.data() + 0x44, "nativePC:test.gmd", 18);
        const auto ma = reinterpret_cast<std::uintptr_t>(mount.data());
        const auto ra = reinterpret_cast<std::uintptr_t>(record.data());
        OpenedSourceSnapshot output;
        assert(capture_opened_source(ra, ma, output) == MemoryReadResult::copied);
        assert(output.mount.name == "nativePC" && output.virtual_request == "nativePC:test.gmd");
        for (std::size_t fail = 1; fail <= 4; ++fail) {
            CopyFault fault; fault.fail_at = fail;
            assert(capture_opened_source(ra, ma, output, copy_fault, &fault) == MemoryReadResult::unreadable);
            assert(output.record_kind == 0);
        }
        record[8] = 1;
        assert(capture_opened_source(ra, ma, output) == MemoryReadResult::rejected);
        record[8] = 0; mount[8] = 3; mount[0x228] = 1;
        assert(capture_opened_source(ra, ma, output) == MemoryReadResult::rejected);
        record[0x28] = 3;
        assert(capture_opened_source(ra, ma, output) == MemoryReadResult::copied);
        record[0x2C] = 7;
        assert(capture_opened_source(ra, ma, output) == MemoryReadResult::rejected);
        assert(output.record_kind == 3);
    }
    {
        std::vector<unsigned char> mount(0x238);
        mount[4] = 1; mount[8] = 1;
        std::memcpy(mount.data() + 0x0C, "nativePC", 9);
        std::memcpy(mount.data() + 0x1C, "mods", 5);
        const auto address = reinterpret_cast<std::uintptr_t>(mount.data());
        MountOriginSnapshot output;
        assert(capture_mount_origin(address, output) == MemoryReadResult::copied);
        assert(output.name == "nativePC" && output.resolved_subpath == "mods");
        for (std::size_t fail = 1; fail <= 2; ++fail) {
            CopyFault fault; fault.fail_at = fail;
            assert(capture_mount_origin(address, output, copy_fault, &fault) == MemoryReadResult::unreadable);
            assert(output.name == "nativePC");
        }
        mount[8] = 3;
        assert(capture_mount_origin(address, output) == MemoryReadResult::rejected);
        mount[0x228] = 1;
        assert(capture_mount_origin(address, output) == MemoryReadResult::copied);
        assert(output.type == 3 && output.container_record == 1);
        std::size_t reads = 0;
        const auto changed = [](void* context, std::uintptr_t location, std::size_t bytes,
            std::size_t maximum, std::vector<unsigned char>& copied) noexcept {
            auto& count = *static_cast<std::size_t*>(context);
            const auto result = copy_current_process_memory(location, bytes, maximum, copied);
            if (result == MemoryReadResult::copied && ++count == 2) copied[0x0C] = 'X';
            return result;
        };
        assert(capture_mount_origin(address, output, changed, &reads) == MemoryReadResult::rejected);
        assert(output.name == "nativePC");
        mount[4] = 0;
        assert(capture_mount_origin(address, output) == MemoryReadResult::rejected);
        mount[4] = 1;
        std::memset(mount.data() + 0x1C, 'x', 260);
        assert(capture_mount_origin(address, output) == MemoryReadResult::rejected);
        assert(output.type == 3 && output.resolved_subpath == "mods");
    }
    {
        std::vector<unsigned char> record(0x148);
        record[0x28] = 1; record[0x2C] = 2;
        std::wstring retained = L"retained";
        assert(capture_native_file_origin(reinterpret_cast<std::uintptr_t>(record.data()), 32768, retained) == MemoryReadResult::rejected);
        record[0x28] = 0;
        assert(capture_native_file_origin(reinterpret_cast<std::uintptr_t>(record.data()), 32768, retained) == MemoryReadResult::rejected);
        assert(retained == L"retained");
    }
    {
        std::vector<unsigned char> stream(24), file(0x114);
        const std::uint64_t stream_vtable = 0x1234, file_vtable = 0x5678, handle = 0x9999;
        const auto file_address = reinterpret_cast<std::uintptr_t>(file.data());
        std::memcpy(stream.data(), &stream_vtable, 8);
        stream[8] = 1;
        std::memcpy(stream.data() + 16, &file_address, 8);
        std::memcpy(file.data(), &file_vtable, 8);
        std::memcpy(file.data() + 8, &handle, 8);
        const std::string path = "nativePC:\\common\\text\\steam\\item_eng.gmd";
        std::memcpy(file.data() + 16, path.c_str(), path.size() + 1);
        const auto address = reinterpret_cast<std::uintptr_t>(stream.data());
        TextStreamOrigin origin;
        assert(capture_text_stream_origin(address, stream_vtable, file_vtable, origin) == MemoryReadResult::copied);
        assert(origin.virtual_path == path && origin.provider_handle == handle);
        for (std::size_t n = 1; n <= 4; ++n) {
            CopyFault fault{0, n, false};
            assert(capture_text_stream_origin(address, stream_vtable, file_vtable, origin, copy_fault, &fault) == MemoryReadResult::unreadable);
            assert(origin.virtual_path == path);
        }
        assert(capture_text_stream_origin(address, stream_vtable + 1, file_vtable, origin) == MemoryReadResult::rejected);
        assert(capture_text_stream_origin(address, stream_vtable, file_vtable + 1, origin) == MemoryReadResult::rejected);
        stream[8] = 0;
        assert(capture_text_stream_origin(address, stream_vtable, file_vtable, origin) == MemoryReadResult::rejected);
        stream[8] = 1;
        std::memset(file.data() + 16, 'x', 260);
        assert(capture_text_stream_origin(address, stream_vtable, file_vtable, origin) == MemoryReadResult::rejected);
        assert(origin.virtual_path == path);
    }
    {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        const auto page_size = static_cast<std::size_t>(info.dwPageSize);
        auto* pages = static_cast<unsigned char*>(VirtualAlloc(nullptr, page_size * 2,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        assert(pages);
        pages[0] = 0x41; pages[page_size - 1] = 0x42;
        std::vector<unsigned char> output{0x99};
        const auto address = reinterpret_cast<std::uintptr_t>(pages);
        assert(copy_current_process_memory(address, page_size, page_size, output) == MemoryReadResult::copied);
        assert(output.front() == 0x41 && output.back() == 0x42);
        const auto retained = output;
        DWORD prior = 0;
        assert(VirtualProtect(pages + page_size, page_size, PAGE_NOACCESS, &prior));
        assert(copy_current_process_memory(address + page_size, 1, page_size, output) == MemoryReadResult::unreadable);
        assert(copy_current_process_memory(address + page_size - 1, 2, page_size, output) == MemoryReadResult::unreadable);
        assert(output == retained);
        assert(copy_current_process_memory(0, 1, page_size, output) == MemoryReadResult::rejected);
        assert(copy_current_process_memory(address, 0, page_size, output) == MemoryReadResult::rejected);
        assert(copy_current_process_memory(address, page_size + 1, page_size, output) == MemoryReadResult::rejected);
        assert(copy_current_process_memory(std::numeric_limits<std::uintptr_t>::max(), 2, page_size, output) == MemoryReadResult::rejected);
        fail_new_countdown = 1;
        assert(copy_current_process_memory(address, 1, page_size, output) == MemoryReadResult::out_of_memory);
        assert(output == retained);
        assert(VirtualFree(pages, 0, MEM_RELEASE));
        assert(copy_current_process_memory(address, 1, page_size, output) == MemoryReadResult::unreadable);
        assert(output == retained);
    }
    {
        GmdCopiedStorage input;
        input.key_pool_address = 0x1000; input.text_pool_address = 0x2000;
        input.key_count = 2; input.text_count = 2;
        input.key_pool = {'A', 0, 'B', 0};
        input.text_pool = {'x', 0, 'y', 0};
        input.records.resize(64); input.text_pointers.resize(16);
        for (std::size_t n = 0; n < 2; ++n) {
            const std::uint32_t index = static_cast<std::uint32_t>(n) | 0x80000000;
            const std::uint64_t key = 0x1000 + n * 2, text = 0x2000 + n * 2;
            std::memcpy(input.records.data() + n * 32, &index, 4);
            std::memcpy(input.records.data() + n * 32 + 16, &key, 8);
            std::memcpy(input.text_pointers.data() + n * 8, &text, 8);
        }
        OwnedTextSnapshot snapshot;
        {
            std::vector<unsigned char> header(0x918);
            const auto put32 = [&](std::size_t offset, std::uint32_t value) { std::memcpy(header.data() + offset, &value, 4); };
            const auto put64 = [&](std::size_t offset, std::uint64_t value) { std::memcpy(header.data() + offset, &value, 8); };
            put64(0, 0x123456); put32(0xA8, 0x10302);
            put32(0xB8, input.key_count); put32(0xD8, input.text_count);
            put32(0xC8, 4); put32(0xE8, 4);
            put64(0xC0, reinterpret_cast<std::uintptr_t>(input.records.data()));
            put64(0xD0, reinterpret_cast<std::uintptr_t>(input.key_pool.data()));
            put64(0xF0, reinterpret_cast<std::uintptr_t>(input.text_pool.data()));
            put64(0xF8, reinterpret_cast<std::uintptr_t>(input.text_pointers.data()));
            GmdCopiedStorage captured;
            const auto address = reinterpret_cast<std::uintptr_t>(header.data());
            assert(capture_gmd_storage(address, 0x123456, 88, captured) == MemoryReadResult::copied);
            assert(captured.records == input.records && captured.text_pool == input.text_pool);
            const auto retained = captured.records;
            for (std::size_t n = 1; n <= 6; ++n) {
                CopyFault fault{0, n, false};
                assert(capture_gmd_storage(address, 0x123456, 88, captured, copy_fault, &fault) == MemoryReadResult::unreadable);
                assert(captured.records == retained);
            }
            CopyFault stale{0, 0, true};
            assert(capture_gmd_storage(address, 0x123456, 88, captured, copy_fault, &stale) == MemoryReadResult::rejected);
            assert(capture_gmd_storage(address, 0x123457, 88, captured) == MemoryReadResult::rejected);
            assert(capture_gmd_storage(address, 0x123456, 87, captured) == MemoryReadResult::rejected);
            for (const std::uint32_t flags : {1u, 16u, 64u}) {
                put32(0x64, flags);
                assert(capture_gmd_storage(address, 0x123456, 88, captured) == MemoryReadResult::rejected);
            }
            put32(0x64, 0); put64(0x900, 1);
            assert(capture_gmd_storage(address, 0x123456, 88, captured) == MemoryReadResult::rejected);
            put64(0x900, 0); put32(0xA8, 0);
            assert(capture_gmd_storage(address, 0x123456, 88, captured) == MemoryReadResult::rejected);
            assert(captured.records == retained);
        }
        assert(decode_gmd_snapshot(input, snapshot) == GmdSnapshotResult::copied);
        assert(snapshot.keys[1].key == "B" && snapshot.keys[1].index == 1 && snapshot.strings[1] == "y");
        for (int scenario = 0; scenario < 7; ++scenario) {
            auto bad = input;
            if (scenario == 0) bad.records.pop_back();
            if (scenario == 1) bad.text_pointers.pop_back();
            if (scenario == 2) bad.text_pool.back() = 'z';
            if (scenario == 3) bad.key_pool[2] = 'A';
            if (scenario == 4) bad.records[0] = 9;
            if (scenario == 5) bad.text_pool_address = 0x2001;
            if (scenario == 6) bad.key_pool_address = std::numeric_limits<std::uint64_t>::max() - 1;
            assert(decode_gmd_snapshot(bad, snapshot) == GmdSnapshotResult::rejected);
            assert(snapshot.keys[1].key == "B" && snapshot.strings[1] == "y");
        }
        fail_new_countdown = 1;
        assert(decode_gmd_snapshot(input, snapshot) == GmdSnapshotResult::out_of_memory);
        assert(snapshot.strings[1] == "y");
        input.key_pool[2] = 'C'; input.text_pool[2] = 'q';
        assert(snapshot.keys[1].key == "B" && snapshot.strings[1] == "y");
    }
    for (const auto& group : text_delta_groups) {
        std::vector<TextKeySnapshot> keys;
        std::vector<std::string_view> strings(group.count + 1, "unrelated MOD text");
        for (std::size_t n = 0; n < group.count; ++n) {
            const auto& d = text_deltas[group.start + n];
            const auto index = group.count - n; // intentionally not delta ordinal
            strings[index] = d.before;
            for (std::size_t k = 0; k < d.key_count; ++k)
                keys.push_back({text_delta_keys[d.key_start + k], index, false});
        }
        keys.push_back({"UNRELATED_MOD_KEY", 0, false});
        std::vector<TextMergeEdit> plan;
        assert(plan_text_merge(group, keys, strings, plan));
        assert(plan.size() == group.count);
        AllocState allocator;
        {
            OwnedTextSnapshot snapshot;
            for (const auto& key : keys) snapshot.keys.push_back({std::string(key.key), key.index});
            for (const auto& value : strings) snapshot.strings.emplace_back(value);
            GmdTextAllocatorContext context{&allocator, allocate_gmd_test, free_test};
            const auto api = make_gmd_text_allocator(context);
            PreparedTextPool result;
            GmdFixture fixture(snapshot);
            const auto address = reinterpret_cast<std::uintptr_t>(fixture.header.data());
            const std::string family = group.family == TextFamily::item ? "item" :
                group.family == TextFamily::lbg ? "lbg" : "wep_series";
            const std::string path = "common/text/steam/" + family + "_" + std::string(text_languages[group.language]);
            std::memcpy(fixture.header.data() + 0x0C, path.c_str(), path.size() + 1);
            assert(prepare_selected_gmd_resource(address, 0x123456, 1024 * 1024, {}, api, result).stage == GmdPrepareStage::prepared);
            for (const auto& edit : plan) assert(std::string_view(result.pointers[edit.index]) == edit.replacement);
            const auto selected_pool = result.pool;
            fixture.header[0x0C] = 'X';
            CopyFault skip_count;
            assert(prepare_selected_gmd_resource(address, 0x123456, 1024 * 1024, {}, api, result, copy_fault, &skip_count).stage == GmdPrepareStage::skipped);
            assert(skip_count.calls == 1 && result.pool == selected_pool);
            fixture.header[0x0C] = 'c';
            CopyFault renamed{0, 0, false, true};
            assert(prepare_selected_gmd_resource(address, 0x123456, 1024 * 1024, {}, api, result, copy_fault, &renamed).stage == GmdPrepareStage::capture_failed);
            assert(result.pool == selected_pool);
            assert(prepare_gmd_resource(address, 0x123456, 1024 * 1024, group, {}, api, result).stage == GmdPrepareStage::prepared);
            for (const auto& edit : plan) assert(std::string_view(result.pointers[edit.index]) == edit.replacement);
            assert(prepare_gmd_resource(address, 0x123456, 1024 * 1024, group,
                {snapshot.keys[0].key}, api, result).stage == GmdPrepareStage::prepared);
            assert(std::string_view(result.pointers[snapshot.keys[0].index]) == snapshot.strings[snapshot.keys[0].index]);
            const auto before_failure = result.pool;
            CopyFault fault{0, 3, false};
            assert(prepare_gmd_resource(address, 0x123456, 1024 * 1024, group, {}, api, result, copy_fault, &fault).stage == GmdPrepareStage::capture_failed);
            const auto key_byte = fixture.key_pool[0];
            fixture.key_pool[0] = 0;
            assert(prepare_gmd_resource(address, 0x123456, 1024 * 1024, group, {}, api, result).stage == GmdPrepareStage::decode_failed);
            fixture.key_pool[0] = key_byte;
            assert(prepare_gmd_resource(address, 0x123456, 1024 * 1024, group, {"unknown override"}, api, result).stage == GmdPrepareStage::prepare_failed);
            assert(result.pool == before_failure && allocator.live == 2);
            assert(prepare_snapshot_transaction(group, snapshot, {}, api, result) == TextPrepareResult::prepared);
            assert(std::string_view(result.pointers[0]) == "unrelated MOD text");
            for (const auto& edit : plan) assert(std::string_view(result.pointers[edit.index]) == edit.replacement);
            const std::vector<std::string_view> overrides{snapshot.keys[0].key};
            assert(prepare_snapshot_transaction(group, snapshot, overrides, api, result) == TextPrepareResult::prepared);
            assert(std::string_view(result.pointers[snapshot.keys[0].index]) == snapshot.strings[snapshot.keys[0].index]);
            const auto retained = result.pool;
            assert(prepare_snapshot_transaction(group, snapshot, {"unknown override key"}, api, result) == TextPrepareResult::rejected);
            assert(prepare_snapshot_transaction(group, snapshot, {overrides[0], overrides[0]}, api, result) == TextPrepareResult::rejected);
            for (const std::size_t failure : {1u, 2u, 3u, 4u}) {
                fail_new_countdown = failure; // key views, string views, plan reserve, value copy
                assert(prepare_snapshot_transaction(group, snapshot, {}, api, result) == TextPrepareResult::out_of_memory);
                assert(fail_new_countdown == 0 && result.pool == retained && allocator.live == 2);
            }
            assert(result.pool == retained);
            snapshot.strings[snapshot.keys[0].index] = "external target override";
            assert(prepare_snapshot_transaction(group, snapshot, {}, api, result) == TextPrepareResult::prepared);
            assert(std::string_view(result.pointers[snapshot.keys[0].index]) == "external target override");
        }
        assert(allocator.live == 0);
        {
            PreparedTextPool prepared;
            GmdTextAllocatorContext context{&allocator, allocate_gmd_test, free_test};
            const auto api = make_gmd_text_allocator(context);
            assert(prepare_text_pool(strings, plan, api, prepared));
            assert(allocator.live == 2 && prepared.count == strings.size());
            assert(std::string_view(prepared.pointers[0]) == "unrelated MOD text");
            for (const auto& edit : plan)
                assert(std::string_view(prepared.pointers[edit.index]) == edit.replacement);
            const auto old_pool = prepared.pool;
            for (const std::size_t failure : {1u, 2u}) {
                fail_new_countdown = failure; // plan reserve, then snapshot copy
                assert(prepare_text_transaction(group, keys, strings, api, prepared) == TextPrepareResult::out_of_memory);
                assert(fail_new_countdown == 0 && prepared.pool == old_pool && allocator.live == 2);
            }
            throw_unexpected = true;
            fail_new_countdown = 1;
            assert(prepare_text_transaction(group, keys, strings, api, prepared) == TextPrepareResult::unexpected_exception);
            throw_unexpected = false;
            assert(prepared.pool == old_pool && allocator.live == 2);
            assert(prepare_text_transaction(group, keys, strings, api, prepared) == TextPrepareResult::prepared);
            const auto transaction_pool = prepared.pool;
            assert(transaction_pool && allocator.live == 2);
            assert(prepare_text_transaction(group, keys, strings, {}, prepared) == TextPrepareResult::rejected);
            assert(prepared.pool == transaction_pool && allocator.live == 2);
            for (const std::size_t failure : {1u, 2u}) {
                allocator.fail_at = allocator.calls + failure;
                assert(!prepare_text_pool(strings, plan, api, prepared));
                assert(prepared.pool == transaction_pool && allocator.live == 2);
            }
            auto stale = plan;
            stale[0].expected = "stale snapshot";
            assert(!prepare_text_pool(strings, stale, api, prepared));
            assert(prepared.pool == transaction_pool && allocator.live == 2);
            auto duplicate = plan;
            duplicate.push_back(plan[0]);
            assert(!prepare_text_pool(strings, duplicate, api, prepared));
            auto embedded = strings;
            embedded[0] = std::string_view("bad\0text", 8);
            assert(!prepare_text_pool(embedded, plan, api, prepared));
            assert(prepared.pool == transaction_pool && allocator.live == 2);
            const auto receipt = prepared.view();
            auto wrong = receipt;
            wrong.bytes += 1;
            assert(!prepared.relinquish_after_commit(wrong));
            assert(prepared.pool == transaction_pool && allocator.live == 2);
            wrong = receipt;
            wrong.allocator.context = nullptr;
            assert(!prepared.relinquish_after_commit(wrong));
            assert(prepared.pool == transaction_pool && allocator.live == 2);
            assert(prepared.relinquish_after_commit(receipt));
            assert(!prepared.relinquish_after_commit(receipt));
            prepared.reset();
            assert(allocator.live == 2); // ownership now belongs to simulated resource
            receipt.allocator.release(receipt.allocator.context, receipt.pointers);
            receipt.allocator.release(receipt.allocator.context, receipt.pool);
            assert(allocator.live == 0);
        }
        assert(allocator.live == 0);
        for (std::size_t n = 0; n < group.count; ++n) {
            assert(plan[n].index == group.count - n);
            assert(plan[n].status == TextMergeStatus::update);
            strings[plan[n].index] = plan[n].replacement;
        }
        assert(strings[0] == "unrelated MOD text");
        assert(plan_text_merge(group, keys, strings, plan));
        for (const auto& p : plan) assert(p.status == TextMergeStatus::already_updated);
        strings[keys[0].index] = "external target override";
        assert(plan_text_merge(group, keys, strings, plan));
        assert(plan[0].status == TextMergeStatus::external_override);
        strings[keys[0].index] = text_deltas[group.start].before;
        keys[0].explicit_mod_override = true;
        assert(plan_text_merge(group, keys, strings, plan));
        assert(plan[0].replacement == text_deltas[group.start].before);
        keys.push_back({"UNRELATED_ALIAS", keys[0].index, false});
        assert(!plan_text_merge(group, keys, strings, plan));
        assert(plan.size() == group.count && plan[0].status == TextMergeStatus::external_override);
        keys.pop_back();
        keys[0].key = "MISSING_TARGET";
        assert(!plan_text_merge(group, keys, strings, plan));
        assert(plan.size() == group.count);
    }
    assert(!valid_gmd_allocation_size(0));
    assert(valid_gmd_allocation_size(std::numeric_limits<std::uint32_t>::max()));
    assert(!valid_gmd_allocation_size(static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1));
    GmdTextAllocatorContext invalid{};
    assert(!make_gmd_text_allocator(invalid).allocate);
    assert(!allocate_gmd_text(nullptr, 8));
    std::cout << "42 groups: full capture/decode/prepare pipeline, MOD overrides, fault stages and ownership cleanup passed\n";
}
