#pragma once
#include "mount_origin_snapshot.h"

namespace stormslinger {
struct OpenedSourceSnapshot {
    MountOriginSnapshot mount;
    std::uint32_t record_kind = 0;
    std::uint64_t opened_identity = 0;
    std::string virtual_request;
};
// Invoke before provider releases its mount reference, with both slot identities verified.
// This binds copied record/mount shapes, not an authenticated physical source.
inline MemoryReadResult capture_opened_source(std::uintptr_t record, std::uintptr_t mount,
    OpenedSourceSnapshot& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    try {
        if (!record || !mount || !copier) return MemoryReadResult::rejected;
        std::vector<unsigned char> before, after;
        auto result = copier(context, record, 0x148, 0x148, before);
        if (result != MemoryReadResult::copied) return result;
        if (before.size() != 0x148 || copied_field<std::uint32_t>(before, 0x2C) != 2)
            return MemoryReadResult::rejected;
        OpenedSourceSnapshot candidate;
        candidate.record_kind = copied_field<std::uint32_t>(before, 0x28);
        candidate.opened_identity = copied_field<std::uint64_t>(before, 0);
        if (!candidate.opened_identity || candidate.opened_identity == UINT64_MAX)
            return MemoryReadResult::rejected;
        const auto* request = reinterpret_cast<const char*>(before.data() + 0x44);
        const auto* end = static_cast<const char*>(std::memchr(request, 0, 260));
        if (!end || end == request) return MemoryReadResult::rejected;
        candidate.virtual_request.assign(request, static_cast<std::size_t>(end - request));
        result = capture_mount_origin(mount, candidate.mount, copier, context);
        if (result != MemoryReadResult::copied) return result;
        if (candidate.mount.type == 3) {
            if (candidate.record_kind != 3) return MemoryReadResult::rejected;
        } else {
            if (candidate.record_kind != 0 || candidate.mount.container_record)
                return MemoryReadResult::rejected;
            for (std::size_t offset : {8u, 16u, 24u, 32u})
                if (copied_field<std::uint64_t>(before, offset)) return MemoryReadResult::rejected;
        }
        result = copier(context, record, 0x148, 0x148, after);
        if (result != MemoryReadResult::copied) return result;
        if (before != after) return MemoryReadResult::rejected;
        output = std::move(candidate);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
