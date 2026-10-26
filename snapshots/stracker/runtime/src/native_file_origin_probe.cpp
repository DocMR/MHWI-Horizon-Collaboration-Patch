#include "native_file_origin.h"
#include "native_file_identity.h"
#include "file_record_identity.h"
#include "source_identity_pipeline.h"
#include <iostream>

int main() {
    std::vector<wchar_t> path(32768);
    DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    std::cerr << "module count=" << count << " error=" << GetLastError() << std::endl;
    if (!count || count >= path.size()) return 2;
    HANDLE handle = CreateFileW(path.data(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    std::cerr << "open success=" << (handle != INVALID_HANDLE_VALUE) << " error=" << GetLastError() << std::endl;
    if (handle == INVALID_HANDLE_VALUE) return 3;
    stormslinger::NativeFileIdentity identity;
    DWORD identity_error = 0;
    const auto identified = stormslinger::query_native_file_identity(handle, identity, identity_error);
    std::cerr << "identity success=" << identified << " error=" << identity_error
        << " volume=" << identity.volume << " index=" << identity.file_index << " size=" << identity.size << std::endl;
    const auto second_handle = CreateFileW(path.data(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    stormslinger::NativeFileIdentity second;
    DWORD second_error = 0;
    const auto second_identified = stormslinger::query_native_file_identity(second_handle, second, second_error);
    const bool same = identified && second_identified && identity == second;
    std::cerr << "second identity success=" << second_identified << " error=" << second_error
        << " same=" << same << std::endl;
    if (second_handle != INVALID_HANDLE_VALUE) CloseHandle(second_handle);
    stormslinger::NativeFileIdentity retained{1, 2, 3, 4};
    DWORD invalid_error = 0;
    const bool invalid = stormslinger::query_native_file_identity(INVALID_HANDLE_VALUE, retained, invalid_error);
    if (invalid || invalid_error != ERROR_INVALID_HANDLE || retained.file_index != 2) return 5;
    const auto required = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_OPENED | VOLUME_NAME_DOS);
    std::cerr << "path required=" << required << " error=" << GetLastError() << std::endl;
    std::vector<unsigned char> record(0x148);
    const auto raw = reinterpret_cast<std::uintptr_t>(handle);
    std::memcpy(record.data(), &raw, 8); record[0x2C] = 2;
    const auto record_address = reinterpret_cast<std::uintptr_t>(record.data());
    stormslinger::NativeFileIdentity from_record;
    DWORD record_error = 0;
    const auto ordinary = stormslinger::capture_file_record_identity(record_address,
        stormslinger::FileRecordShape::ordinary_read, from_record, record_error);
    if (ordinary != stormslinger::MemoryReadResult::copied || !(from_record == identity)) return 7;
    record[0x2C] = 7;
    std::memcpy(record.data() + 0x20, &record_address, 8);
    const auto container = stormslinger::capture_file_record_identity(record_address,
        stormslinger::FileRecordShape::constructed_container_source, from_record, record_error);
    if (container != stormslinger::MemoryReadResult::copied || !(from_record == identity)) return 8;
    record[0x28] = 3;
    if (stormslinger::capture_file_record_identity(record_address, stormslinger::FileRecordShape::constructed_container_source,
        from_record, record_error) != stormslinger::MemoryReadResult::rejected || !(from_record == identity)) return 9;
    record[0x28] = 0; record[0x2C] = 2;
    std::memset(record.data() + 0x20, 0, 8);
    for (std::size_t fail = 1; fail <= 2; ++fail) {
        struct Fault { std::size_t calls, fail; } fault{0, fail};
        const auto copy = [](void* context, std::uintptr_t address, std::size_t bytes,
            std::size_t maximum, std::vector<unsigned char>& output) noexcept {
            auto& state = *static_cast<Fault*>(context);
            if (++state.calls == state.fail) return stormslinger::MemoryReadResult::unreadable;
            return stormslinger::copy_current_process_memory(address, bytes, maximum, output);
        };
        if (stormslinger::capture_file_record_identity(record_address, stormslinger::FileRecordShape::ordinary_read,
            from_record, record_error, copy, &fault) != stormslinger::MemoryReadResult::unreadable ||
            !(from_record == identity)) return 10;
    }
    std::cerr << "record identity ordinary=1 container-source=1 wrong-kind-rejected=1" << std::endl;
    std::vector<unsigned char> mount(0x238), entry(0x148), source_record(0x148);
    mount[4] = 1; mount[8] = 1;
    std::memcpy(mount.data() + 0x0C, "nativePC", 9);
    std::memcpy(record.data() + 0x44, "nativePC:test.gmd", 18);
    const auto mount_address = reinterpret_cast<std::uintptr_t>(mount.data());
    stormslinger::SourceIdentitySnapshot combined;
    if (stormslinger::capture_source_identity(record_address, mount_address, combined, record_error) !=
        stormslinger::MemoryReadResult::copied || !(combined.identity == identity)) return 11;
    std::memcpy(source_record.data(), &raw, 8); source_record[0x2C] = 7;
    const auto source_address = reinterpret_cast<std::uintptr_t>(source_record.data());
    std::memcpy(source_record.data() + 0x20, &source_address, 8);
    mount[8] = 3; std::memcpy(mount.data() + 0x228, &source_address, 8);
    entry[0] = 1; entry[0x28] = 3; entry[0x2C] = 2;
    std::memcpy(entry.data() + 0x44, "native:test.gmd", 16);
    const auto entry_address = reinterpret_cast<std::uintptr_t>(entry.data());
    if (stormslinger::capture_source_identity(entry_address, mount_address, combined, record_error) !=
        stormslinger::MemoryReadResult::copied || !(combined.identity == identity)) return 12;
    source_record[0x28] = 3;
    if (stormslinger::capture_source_identity(entry_address, mount_address, combined, record_error) !=
        stormslinger::MemoryReadResult::rejected || combined.source.record_kind != 3) return 13;
    std::cerr << "source pipeline ordinary=1 container=1 unsupported-source-rejected=1" << std::endl;
    std::vector<unsigned char> copied;
    auto read = stormslinger::copy_current_process_memory(reinterpret_cast<std::uintptr_t>(record.data()),
        record.size(), record.size(), copied);
    std::cerr << "record read=" << static_cast<int>(read) << " error=" << GetLastError() << std::endl;
    std::wstring result = L"retained";
    stormslinger::NativeOriginDiagnostic diagnostic;
    auto status = stormslinger::capture_native_file_origin(reinterpret_cast<std::uintptr_t>(record.data()), 32768, result,
        stormslinger::default_memory_copier, nullptr, &diagnostic);
    std::cerr << "capture=" << static_cast<int>(status) << " stage=" << static_cast<int>(diagnostic.stage)
        << " error=" << diagnostic.win32_error << " chars=" << result.size() << std::endl;
    const auto closed = CloseHandle(handle);
    std::cerr << "close=" << closed << std::endl;
    if (!same) return 6;
    if (status == stormslinger::MemoryReadResult::copied) return 0;
    if (status == stormslinger::MemoryReadResult::unreadable && result == L"retained" &&
        diagnostic.stage == stormslinger::NativeOriginStage::path_size && diagnostic.win32_error == ERROR_ACCESS_DENIED)
        return 0; // verifies diagnosed failure preservation, not successful path querying
    return 4;
}
