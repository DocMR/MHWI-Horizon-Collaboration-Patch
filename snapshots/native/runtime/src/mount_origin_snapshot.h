#pragma once
#include "gmd_storage_capture.h"

namespace stormslinger {
struct MountOriginSnapshot {
    std::uint32_t name_hash = 0, type = 0;
    std::uint64_t container_record = 0, index_identity = 0;
    std::string name, resolved_subpath, optional_prefix;
};
// Caller must prove a held mount reference and validated manager slot identity.
// Numeric identities are diagnostic tokens, never dereferenced after return.
inline MemoryReadResult capture_mount_origin(std::uintptr_t mount,
    MountOriginSnapshot& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    try {
        if (!mount || !copier) return MemoryReadResult::rejected;
        std::vector<unsigned char> before, after;
        auto result = copier(context, mount, 0x238, 0x238, before);
        if (result != MemoryReadResult::copied) return result;
        if (before.size() != 0x238 || copied_field<std::int32_t>(before, 4) <= 0)
            return MemoryReadResult::rejected;
        MountOriginSnapshot candidate;
        candidate.name_hash = copied_field<std::uint32_t>(before, 0);
        candidate.type = copied_field<std::uint32_t>(before, 8);
        candidate.container_record = copied_field<std::uint64_t>(before, 0x228);
        candidate.index_identity = copied_field<std::uint64_t>(before, 0x230);
        if (candidate.type < 1 || candidate.type > 3 ||
            (candidate.type == 3 && !candidate.container_record)) return MemoryReadResult::rejected;
        const auto string_at = [&](std::size_t offset, std::size_t limit, std::string& value) {
            const auto* start = reinterpret_cast<const char*>(before.data() + offset);
            const auto* end = static_cast<const char*>(std::memchr(start, 0, limit));
            if (!end) return false;
            value.assign(start, static_cast<std::size_t>(end - start));
            return true;
        };
        if (!string_at(0x0C, 16, candidate.name) || candidate.name.empty() ||
            !string_at(0x1C, 260, candidate.resolved_subpath) ||
            !string_at(0x120, 260, candidate.optional_prefix)) return MemoryReadResult::rejected;
        result = copier(context, mount, 0x238, 0x238, after);
        if (result != MemoryReadResult::copied) return result;
        if (before != after) return MemoryReadResult::rejected;
        output = std::move(candidate);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
