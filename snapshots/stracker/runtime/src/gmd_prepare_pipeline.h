#pragma once
#include "gmd_storage_capture.h"
#include "text_snapshot_transaction.h"

namespace stormslinger {
enum class GmdPrepareStage { capture_failed, decode_failed, prepare_failed, prepared, skipped };
struct GmdPrepareOutcome {
    GmdPrepareStage stage = GmdPrepareStage::capture_failed;
    MemoryReadResult capture = MemoryReadResult::rejected;
    GmdSnapshotResult decode = GmdSnapshotResult::rejected;
    TextPrepareResult prepare = TextPrepareResult::rejected;
};
// Precondition: verified resource/build/path identity, source evidence and read boundary.
// This prepares owned buffers only. It does not publish fields or install a hook.
inline GmdPrepareOutcome prepare_gmd_resource_impl(std::uintptr_t resource, std::uint64_t vtable,
    std::size_t maximum_payload, const TextDeltaGroup* explicit_group,
    const std::vector<std::string_view>& override_keys, TextAllocator allocator,
    PreparedTextPool& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    GmdPrepareOutcome outcome;
    GmdCopiedStorage storage;
    outcome.capture = capture_gmd_storage(resource, vtable, maximum_payload, storage, copier, context, !explicit_group);
    if (outcome.capture == MemoryReadResult::not_target) { outcome.stage = GmdPrepareStage::skipped; return outcome; }
    if (outcome.capture != MemoryReadResult::copied) return outcome;
    const auto* selected_group = explicit_group ? explicit_group : find_text_deltas(
        select_text_resource(storage.resource_name.data(), storage.resource_name.size()));
    if (!selected_group) return outcome;
    outcome.stage = GmdPrepareStage::decode_failed;
    OwnedTextSnapshot snapshot;
    outcome.decode = decode_gmd_snapshot(storage, snapshot);
    if (outcome.decode != GmdSnapshotResult::copied) return outcome;
    outcome.stage = GmdPrepareStage::prepare_failed;
    outcome.prepare = prepare_snapshot_transaction(*selected_group, snapshot, override_keys, allocator, output);
    if (outcome.prepare == TextPrepareResult::prepared) outcome.stage = GmdPrepareStage::prepared;
    return outcome;
}
inline GmdPrepareOutcome prepare_gmd_resource(std::uintptr_t resource, std::uint64_t vtable,
    std::size_t maximum_payload, const TextDeltaGroup& group,
    const std::vector<std::string_view>& override_keys, TextAllocator allocator,
    PreparedTextPool& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    return prepare_gmd_resource_impl(resource, vtable, maximum_payload, &group, override_keys,
        allocator, output, copier, context);
}
inline GmdPrepareOutcome prepare_selected_gmd_resource(std::uintptr_t resource, std::uint64_t vtable,
    std::size_t maximum_payload, const std::vector<std::string_view>& override_keys,
    TextAllocator allocator, PreparedTextPool& output, MemoryCopier copier = default_memory_copier,
    void* context = nullptr) noexcept {
    return prepare_gmd_resource_impl(resource, vtable, maximum_payload, nullptr,
        override_keys, allocator, output, copier, context);
}
} // namespace stormslinger
