#pragma once
#include "gmd_storage_capture.h"
#include "native_file_identity.h"

namespace stormslinger {
enum class FileRecordShape { ordinary_read, constructed_container_source };
// Caller verifies manager identity and keeps record/handle alive throughout.
inline MemoryReadResult capture_file_record_identity(std::uintptr_t record, FileRecordShape shape,
    NativeFileIdentity& output, DWORD& error, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    error = ERROR_SUCCESS;
    try {
        if (!record || !copier) return MemoryReadResult::rejected;
        std::vector<unsigned char> before, after;
        auto result = copier(context, record, 0x148, 0x148, before);
        if (result != MemoryReadResult::copied) return result;
        if (before.size() != 0x148 || copied_field<std::uint32_t>(before, 0x28) != 0)
            return MemoryReadResult::rejected;
        const auto mode = copied_field<std::uint32_t>(before, 0x2C);
        if (shape == FileRecordShape::ordinary_read) {
            if (mode != 2) return MemoryReadResult::rejected;
            for (std::size_t offset : {8u, 16u, 24u, 32u})
                if (copied_field<std::uint64_t>(before, offset)) return MemoryReadResult::rejected;
        } else if (shape == FileRecordShape::constructed_container_source) {
            if (mode != 7 || copied_field<std::uint64_t>(before, 0x20) != record)
                return MemoryReadResult::rejected;
        } else return MemoryReadResult::rejected;
        const auto raw = copied_field<std::uint64_t>(before, 0);
        NativeFileIdentity candidate;
        if (!query_native_file_identity(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(raw)), candidate, error))
            return MemoryReadResult::unreadable;
        result = copier(context, record, 0x148, 0x148, after);
        if (result != MemoryReadResult::copied) return result;
        if (before != after) return MemoryReadResult::rejected;
        output = candidate;
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
