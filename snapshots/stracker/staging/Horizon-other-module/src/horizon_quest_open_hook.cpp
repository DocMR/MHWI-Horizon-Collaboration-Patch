#include "horizon_quest_open_hook.h"
#include "horizon_quest_open_maintenance.h"
#include "horizon_quest_catalog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
namespace horizon_other {
namespace {
#include "generated_quest_open_getter_owner.inc"
NativeQuestOpenMaintenance maintenance;
const PreparedQuestMapping* mapping_state = nullptr;
void* original_open = nullptr;
bool attempted = false;
using OpenGetter = bool (*)(void*, std::int32_t);
bool open_hook(void* character, std::int32_t id) {
    if (find_quest(id)) {
        if (!mapping_state || maintenance.known_requirements(character, id, *mapping_state) != QuestRequirementResult::requirements_met ||
            maintenance.refresh(character, *mapping_state).status == QuestOpenRefreshStatus::unavailable)
            return false;
    }
    return reinterpret_cast<OpenGetter>(original_open)(character, id);
}
}
EquipmentQueueResult queue_quest_open_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, const PreparedQuestMapping& mapping) noexcept {
    if (!image || !mapping.table() || mapping.image() != image || !transaction.can_add_hooks())
        return EquipmentQueueResult::invalid_host;
    if (attempted) return EquipmentQueueResult::already_attempted;
    if (!verify_native_owner(image, kNativeOpenGetterOwner) || !maintenance.bind(image))
        return EquipmentQueueResult::owner_mismatch;
    attempted = true;
    mapping_state = &mapping;
    if (transaction.add(const_cast<std::uint8_t*>(image)+0x134F070,
                        reinterpret_cast<void*>(&open_hook), &original_open) != MH_OK)
        return EquipmentQueueResult::hook_failed;
    return EquipmentQueueResult::queued;
}
}
