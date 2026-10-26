#pragma once
#include "horizon_native_integration.h"
#include "horizon_owner_verification.h"

namespace stormslinger { class HookTransaction; }
namespace horizon_other {
using OwnerVerifier = bool (*)(const std::uint8_t* image, const TreeOwnerSpec& owner) noexcept;
enum class EquipmentQueueResult { queued, invalid_host, already_attempted, owner_mismatch, clone_failed, hook_failed };
// Host owns one shared transaction, certification, commit/rollback and DLL lifetime.
// Call once, before equipment population, on the serialized bootstrap thread.
// Shared composers must accept both bow owners and preserve this module's edits.
// Also queues the two-series native layered ownership mapper adaptation.
// Source integration does not certify the production entry or borrowed lifetimes.
EquipmentQueueResult queue_equipment_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* certified_image, OwnerVerifier verify_owner = &verify_native_owner,
    CloneComposer shared_bow_edits = nullptr) noexcept;
}
