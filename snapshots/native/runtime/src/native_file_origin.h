#pragma once
#include "text_stream_origin.h"

namespace stormslinger {
enum class NativeOriginStage { validation, record_read, path_size, path_read, record_recheck, complete };
struct NativeOriginDiagnostic { NativeOriginStage stage = NativeOriginStage::validation; DWORD win32_error = 0; };
// Requires verified manager-record identity/lifetime. Never closes the game's handle.
// Returns the opened path, not a normalized/canonical ownership proof.
inline MemoryReadResult capture_native_file_origin(std::uintptr_t record,
    std::size_t maximum_path_chars, std::wstring& output,
    MemoryCopier copier = default_memory_copier, void* context = nullptr,
    NativeOriginDiagnostic* diagnostic = nullptr) noexcept {
    if (diagnostic) *diagnostic = {};
    const auto stage = [&](NativeOriginStage value, DWORD error = 0) noexcept {
        if (diagnostic) *diagnostic = {value, error};
    };
    try {
        if (!copier || !maximum_path_chars || maximum_path_chars > std::numeric_limits<DWORD>::max())
            return MemoryReadResult::rejected;
        std::vector<unsigned char> before, after;
        stage(NativeOriginStage::record_read);
        auto result = copier(context, record, 0x148, 0x148, before);
        if (result != MemoryReadResult::copied) return result;
        stage(NativeOriginStage::validation);
        if (before.size() != 0x148 || copied_field<std::uint32_t>(before, 0x28) != 0 ||
            copied_field<std::uint32_t>(before, 0x2C) != 2) return MemoryReadResult::rejected;
        for (const auto offset : {8u, 16u, 24u, 32u})
            if (copied_field<std::uint64_t>(before, offset)) return MemoryReadResult::rejected;
        const auto raw = copied_field<std::uint64_t>(before, 0);
        if (!raw || raw == std::numeric_limits<std::uint64_t>::max()) return MemoryReadResult::rejected;
        const auto handle = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(raw));
        stage(NativeOriginStage::path_size);
        const auto required = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_OPENED | VOLUME_NAME_DOS);
        if (!required) { stage(NativeOriginStage::path_size, GetLastError()); return MemoryReadResult::unreadable; }
        if (required > maximum_path_chars) return MemoryReadResult::rejected;
        std::vector<wchar_t> path(required);
        stage(NativeOriginStage::path_read);
        const auto written = GetFinalPathNameByHandleW(handle, path.data(), required, FILE_NAME_OPENED | VOLUME_NAME_DOS);
        if (!written) { stage(NativeOriginStage::path_read, GetLastError()); return MemoryReadResult::unreadable; }
        if (written >= required) return MemoryReadResult::unreadable;
        stage(NativeOriginStage::record_recheck);
        result = copier(context, record, 0x148, 0x148, after);
        if (result != MemoryReadResult::copied) return result;
        if (before != after) return MemoryReadResult::rejected;
        std::wstring prepared(path.data(), written);
        output.swap(prepared);
        stage(NativeOriginStage::complete);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
