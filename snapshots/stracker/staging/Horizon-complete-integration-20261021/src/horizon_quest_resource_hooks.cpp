#include "horizon_quest_resource_hooks.h"
#include "horizon_quest_resource_bridge.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
namespace horizon_other {
namespace {
#include "generated_quest_resource_owners.inc"
using Loader = bool (*)(void*, void*);
void* original_list = nullptr;
void* original_data = nullptr;
QuestListApi api{};
bool attempted = false;
bool classification_attempted = false;
void* original_category = nullptr;
using Category = std::int64_t (*)(std::int32_t, std::uint8_t);
std::int64_t category_hook(std::int32_t id, std::uint8_t mode) {
    return find_quest(id) ? 1 : reinterpret_cast<Category>(original_category)(id, mode);
}
bool list_hook(void* resource, void* stream) {
    if (!reinterpret_cast<Loader>(original_list)(resource, stream)) return false;
    switch (append_quest_resource_nodes(resource, api)) {
    case AppendQuestResult::unrelated_resource:
    case AppendQuestResult::unchanged:
    case AppendQuestResult::appended:
        return true;
    case AppendQuestResult::invalid_resource:
    case AppendQuestResult::missing_api:
    case AppendQuestResult::allocation_failed:
    case AppendQuestResult::growth_failed:
        return false;
    }
    return false;
}
bool data_hook(void* resource, void* stream) {
    const bool loaded = reinterpret_cast<Loader>(original_data)(resource, stream);
    if (loaded && resource) assign_loaded_quest_id(resource);
    return loaded;
}
}
EquipmentQueueResult queue_quest_resource_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, OwnerVerifier verify) noexcept {
    if (!image || !verify || !transaction.can_add_hooks()) return EquipmentQueueResult::invalid_host;
    if (attempted) return EquipmentQueueResult::already_attempted;
    for (std::size_t i = 0; i < 5; ++i)
        if (!verify(image, kQuestResourceOwners[i])) return EquipmentQueueResult::owner_mismatch;
    attempted = true;
    api = bind_quest_list_api(image);
    if (transaction.add(const_cast<std::uint8_t*>(image)+0x478A20,
                        reinterpret_cast<void*>(&list_hook), &original_list) != MH_OK ||
        transaction.add(const_cast<std::uint8_t*>(image)+0x1A10170,
                        reinterpret_cast<void*>(&data_hook), &original_data) != MH_OK)
        return EquipmentQueueResult::hook_failed;
    return EquipmentQueueResult::queued;
}
EquipmentQueueResult queue_quest_classification_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, OwnerVerifier verify) noexcept {
    if (!image || !verify || !transaction.can_add_hooks()) return EquipmentQueueResult::invalid_host;
    if (classification_attempted) return EquipmentQueueResult::already_attempted;
    if (!verify(image, kQuestResourceOwners[5])) return EquipmentQueueResult::owner_mismatch;
    classification_attempted = true;
    if (transaction.add(const_cast<std::uint8_t*>(image)+0x19751B0,
                        reinterpret_cast<void*>(&category_hook), &original_category) != MH_OK)
        return EquipmentQueueResult::hook_failed;
    return EquipmentQueueResult::queued;
}
}
