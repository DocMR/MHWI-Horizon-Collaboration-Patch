#pragma once
#include "opened_source_snapshot.h"
#include "file_record_identity.h"

namespace stormslinger {
struct SourceIdentitySnapshot {
    OpenedSourceSnapshot source;
    NativeFileIdentity identity;
};
inline bool same_opened_source(const OpenedSourceSnapshot& a, const OpenedSourceSnapshot& b) noexcept {
    return a.record_kind == b.record_kind && a.opened_identity == b.opened_identity &&
        a.virtual_request == b.virtual_request && a.mount.name_hash == b.mount.name_hash &&
        a.mount.type == b.mount.type && a.mount.container_record == b.mount.container_record &&
        a.mount.index_identity == b.mount.index_identity && a.mount.name == b.mount.name &&
        a.mount.resolved_subpath == b.mount.resolved_subpath && a.mount.optional_prefix == b.mount.optional_prefix;
}
// Caller guarantees record/mount/container association and lifetime. No ownership inference.
inline MemoryReadResult capture_source_identity(std::uintptr_t record, std::uintptr_t mount,
    SourceIdentitySnapshot& output, DWORD& error, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    error = ERROR_SUCCESS;
    try {
        SourceIdentitySnapshot candidate;
        auto result = capture_opened_source(record, mount, candidate.source, copier, context);
        if (result != MemoryReadResult::copied) return result;
        const bool container = candidate.source.mount.type == 3;
        const auto source_record = container ? static_cast<std::uintptr_t>(candidate.source.mount.container_record) : record;
        result = capture_file_record_identity(source_record,
            container ? FileRecordShape::constructed_container_source : FileRecordShape::ordinary_read,
            candidate.identity, error, copier, context);
        if (result != MemoryReadResult::copied) return result;
        OpenedSourceSnapshot after;
        result = capture_opened_source(record, mount, after, copier, context);
        if (result != MemoryReadResult::copied) return result;
        if (!same_opened_source(candidate.source, after)) return MemoryReadResult::rejected;
        output = std::move(candidate);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
