#pragma once
#include "horizon_equipment_hooks.h"
#include "horizon_quest_mapping.h"
namespace horizon_other {
// Adapt owned flag2 admission and condition12 to registered permanent optional
// resources. Preserve other conditions and native unlock/notification/save writers.
// Pose13's original open-bit query also bypasses only PC's index12 exclusion.
EquipmentQueueResult queue_guildcard_admission_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image,const PreparedQuestMapping& mapping) noexcept;
}
