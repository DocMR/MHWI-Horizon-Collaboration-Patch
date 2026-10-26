#pragma once
#include "gmd_snapshot.h"
#include "safe_memory_read.h"
#include "text_resource_selector.h"
#include <array>

namespace stormslinger {
static_assert(sizeof(std::uintptr_t) == 8 && sizeof(std::size_t) == 8, "GMD capture requires the x64 game ABI");
using MemoryCopier = MemoryReadResult (*)(void*, std::uintptr_t, std::size_t,
    std::size_t, std::vector<unsigned char>&) noexcept;
inline MemoryReadResult default_memory_copier(void*, std::uintptr_t address, std::size_t bytes,
    std::size_t maximum, std::vector<unsigned char>& output) noexcept {
    return copy_current_process_memory(address, bytes, maximum, output);
}
template<class T> inline T copied_field(const std::vector<unsigned char>& header, std::size_t offset) noexcept {
    T value{};
    std::memcpy(&value, header.data() + offset, sizeof(value));
    return value;
}
// Call only with verified build/type identity and a live resource read boundary.
// Header equality detects some changes; it is not a lock or an ABA/concurrency proof.
inline MemoryReadResult capture_gmd_storage(std::uintptr_t resource, std::uint64_t expected_vtable,
    std::size_t maximum_payload, GmdCopiedStorage& output,
    MemoryCopier copier = default_memory_copier, void* context = nullptr,
    bool targets_only = false) noexcept {
    try {
        if (!copier || !expected_vtable || !maximum_payload) return MemoryReadResult::rejected;
        std::vector<unsigned char> header;
        auto status = copier(context, resource, 0x918, 0x918, header);
        if (status != MemoryReadResult::copied) return status;
        if (header.size() != 0x918 || copied_field<std::uint64_t>(header, 0) != expected_vtable ||
            copied_field<std::uint32_t>(header, 0xA8) != 0x10302 ||
            (copied_field<std::uint32_t>(header, 0x64) & 0x51) ||
            copied_field<std::uint64_t>(header, 0x900)) return MemoryReadResult::rejected;
        GmdCopiedStorage captured;
        std::memcpy(captured.resource_name.data(), header.data() + 0x0C, 79);
        if (targets_only && !select_text_resource(captured.resource_name.data(), 79))
            return MemoryReadResult::not_target;
        captured.key_count = copied_field<std::uint32_t>(header, 0xB8);
        captured.text_count = copied_field<std::uint32_t>(header, 0xD8);
        const auto key_bytes = copied_field<std::uint32_t>(header, 0xC8);
        const auto text_bytes = copied_field<std::uint32_t>(header, 0xE8);
        const std::uint64_t total = static_cast<std::uint64_t>(captured.key_count) * 32 +
            static_cast<std::uint64_t>(captured.text_count) * 8 + key_bytes + text_bytes;
        if (!captured.key_count || !captured.text_count || !key_bytes || !text_bytes ||
            total > maximum_payload) return MemoryReadResult::rejected;
        captured.key_pool_address = copied_field<std::uint64_t>(header, 0xD0);
        captured.text_pool_address = copied_field<std::uint64_t>(header, 0xF0);
        const std::array<std::uint64_t, 4> addresses{
            copied_field<std::uint64_t>(header, 0xC0), captured.key_pool_address,
            captured.text_pool_address, copied_field<std::uint64_t>(header, 0xF8)};
        const std::array<std::size_t, 4> sizes{static_cast<std::size_t>(captured.key_count) * 32,
            key_bytes, text_bytes, static_cast<std::size_t>(captured.text_count) * 8};
        const std::array<std::vector<unsigned char>*, 4> buffers{
            &captured.records, &captured.key_pool, &captured.text_pool, &captured.text_pointers};
        for (std::size_t n = 0; n < buffers.size(); ++n) {
            status = copier(context, static_cast<std::uintptr_t>(addresses[n]), sizes[n], maximum_payload, *buffers[n]);
            if (status != MemoryReadResult::copied) return status;
            if (buffers[n]->size() != sizes[n]) return MemoryReadResult::rejected;
        }
        std::vector<unsigned char> after;
        status = copier(context, resource, 0x918, 0x918, after);
        if (status != MemoryReadResult::copied) return status;
        if (after.size() != header.size()) return MemoryReadResult::rejected;
        if (std::memcmp(header.data() + 0x0C, after.data() + 0x0C, 79)) return MemoryReadResult::rejected;
        for (const auto offset : {0u, 0x64u, 0xA8u, 0xB8u, 0xC0u, 0xC8u, 0xD0u,
            0xD8u, 0xE8u, 0xF0u, 0xF8u, 0x900u}) {
            const auto width = (offset == 0 || offset == 0xC0 || offset == 0xD0 ||
                offset == 0xF0 || offset == 0xF8 || offset == 0x900) ? 8u : 4u;
            if (std::memcmp(header.data() + offset, after.data() + offset, width)) return MemoryReadResult::rejected;
        }
        output = std::move(captured);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
