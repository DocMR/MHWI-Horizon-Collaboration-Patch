#pragma once
#include "horizon_equipment_hooks.h"
namespace horizon_other {
// Resource loading only; does not install category/open/save-state hooks.
EquipmentQueueResult queue_quest_resource_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* certified_image, OwnerVerifier verify = &verify_native_owner) noexcept;
// Optional-category routing only, separate from resource hooks. Native rank,
// star, open, admission and save predicates must remain intact.
EquipmentQueueResult queue_quest_classification_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* certified_image, OwnerVerifier verify = &verify_native_owner) noexcept;
}
