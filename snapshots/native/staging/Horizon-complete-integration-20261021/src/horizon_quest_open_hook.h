#pragma once
#include "horizon_equipment_hooks.h"
#include "horizon_quest_mapping.h"
namespace horizon_other {
// Queue before mapping edits. Host retains mapping/DLL for process lifetime,
// installs all mapping references before committing the shared Hook transaction,
// and coordinates character load/reset lifetime with native open queries.
EquipmentQueueResult queue_quest_open_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* certified_image, const PreparedQuestMapping& mapping) noexcept;
}
