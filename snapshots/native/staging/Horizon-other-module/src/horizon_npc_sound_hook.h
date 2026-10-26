#pragma once
#include "horizon_equipment_hooks.h"
namespace horizon_other {
EquipmentQueueResult queue_npc_sound_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image) noexcept;
}
