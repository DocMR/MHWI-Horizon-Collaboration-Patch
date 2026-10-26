#include "horizon_equipment_hooks.h"
#include "horizon_native_clone.h"
#include "horizon_armor_exclusions.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../../runtime/src/hooks.h"

namespace horizon_other {
namespace {
PreparedNativeClone clones[3];
bool attempted = false;
void* armor_original = nullptr;
void* layered_mapper_original = nullptr;
// Saved PC original 16B6E60..16B6E9C, including both return paths.
constexpr TreeOwnerSpec kLayeredMapperOwner = {0x16B6E60, 60,
    {0x00, 0x22, 0x9B, 0xEA, 0xA2, 0x30, 0x35, 0xA7,
     0x83, 0x73, 0xBA, 0xF7, 0xCF, 0x71, 0x1D, 0x38,
     0x7B, 0x20, 0xDA, 0xB6, 0xA8, 0xC1, 0x69, 0xD1,
     0xC7, 0xCB, 0x67, 0x70, 0xEC, 0x84, 0x18, 0x26}};
void* unused_originals[3]{};
std::int32_t layered_mapper_hook(std::int32_t series) {
    // Share native ownership between acquisition and duplicate-craft queries.
    // The old delivery 57 remains excluded on both platforms.
    if (series == 154) return 1235;
    if (series == 668) return 3418;
    return reinterpret_cast<std::int32_t (*)(std::int32_t)>(layered_mapper_original)(series);
}
bool armor_exclusion_hook(void* manager, const void* recipe) {
    // Preserve the C++ recipe argument across the original call; RDX is volatile.
    const bool excluded = reinterpret_cast<OriginalArmorPredicate>(armor_original)(manager, recipe);
    return horizon_armor_exclusion_result(recipe, excluded);
}
}
EquipmentQueueResult queue_equipment_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, OwnerVerifier verify, CloneComposer shared) noexcept {
    if (!image || !verify || !transaction.can_add_hooks()) return EquipmentQueueResult::invalid_host;
    if (attempted) return EquipmentQueueResult::already_attempted;
    const auto plan = native_equipment_integration_plan();
    for (const auto& binding : plan.clone_edits)
        if (!binding.owner || !verify(image, *binding.owner)) return EquipmentQueueResult::owner_mismatch;
    if (!plan.armor_post_call.owner || !verify(image, *plan.armor_post_call.owner))
        return EquipmentQueueResult::owner_mismatch;
    if (!verify(image, kLayeredMapperOwner))
        return EquipmentQueueResult::owner_mismatch;
    attempted = true;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto& binding = plan.clone_edits[i];
        const auto* spec = native_clone_spec(binding.owner->rva);
        if (!spec || !clones[i].prepare(*spec, image, binding.compose, i < 2 ? shared : nullptr))
            return EquipmentQueueResult::clone_failed;
    }
    // add() only queues activation. The host commits after all modules are ready.
    for (std::size_t i = 0; i < 3; ++i) {
        auto* target = const_cast<std::uint8_t*>(image) + plan.clone_edits[i].owner->rva;
        if (transaction.add(target, clones[i].entry(), &unused_originals[i]) != MH_OK)
            return EquipmentQueueResult::hook_failed;
    }
    auto* armor_target = const_cast<std::uint8_t*>(image) + plan.armor_post_call.owner->rva;
    if (transaction.add(armor_target, reinterpret_cast<void*>(&armor_exclusion_hook), &armor_original) != MH_OK)
        return EquipmentQueueResult::hook_failed;
    auto* layered_target = const_cast<std::uint8_t*>(image) + kLayeredMapperOwner.rva;
    if (transaction.add(layered_target, reinterpret_cast<void*>(&layered_mapper_hook), &layered_mapper_original) != MH_OK)
        return EquipmentQueueResult::hook_failed;
    return EquipmentQueueResult::queued;
}
}
