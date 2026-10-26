#pragma once
#include "gmd_storage_capture.h"

namespace stormslinger {
struct TextStreamOrigin {
    std::uintptr_t adapter = 0;
    std::uint64_t provider_handle = 0;
    std::string virtual_path;
};
// Records the opened virtual path, not the resolved disk/package owner or MOD intent.
inline MemoryReadResult capture_text_stream_origin(std::uintptr_t stream,
    std::uint64_t expected_stream_vtable, std::uint64_t expected_adapter_vtable,
    TextStreamOrigin& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    try {
        if (!copier || !expected_stream_vtable || !expected_adapter_vtable) return MemoryReadResult::rejected;
        std::vector<unsigned char> head, file, after_file, after_head;
        auto status = copier(context, stream, 24, 24, head);
        if (status != MemoryReadResult::copied) return status;
        if (head.size() != 24 || copied_field<std::uint64_t>(head, 0) != expected_stream_vtable ||
            !(copied_field<std::uint32_t>(head, 8) & 1)) return MemoryReadResult::rejected;
        const auto adapter = copied_field<std::uint64_t>(head, 16);
        status = copier(context, static_cast<std::uintptr_t>(adapter), 0x114, 0x114, file);
        if (status != MemoryReadResult::copied) return status;
        if (file.size() != 0x114 || copied_field<std::uint64_t>(file, 0) != expected_adapter_vtable ||
            !copied_field<std::uint64_t>(file, 8)) return MemoryReadResult::rejected;
        const auto* path = reinterpret_cast<const char*>(file.data() + 16);
        const auto* end = static_cast<const char*>(std::memchr(path, 0, 260));
        if (!end || end == path) return MemoryReadResult::rejected;
        TextStreamOrigin origin;
        origin.adapter = static_cast<std::uintptr_t>(adapter);
        origin.provider_handle = copied_field<std::uint64_t>(file, 8);
        origin.virtual_path.assign(path, static_cast<std::size_t>(end - path));
        status = copier(context, origin.adapter, 0x114, 0x114, after_file);
        if (status != MemoryReadResult::copied) return status;
        status = copier(context, stream, 24, 24, after_head);
        if (status != MemoryReadResult::copied) return status;
        if (file != after_file || head != after_head) return MemoryReadResult::rejected;
        output = std::move(origin);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
