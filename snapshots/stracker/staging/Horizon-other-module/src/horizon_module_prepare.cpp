#include "horizon_module_prepare.h"
#include "horizon_quest_resource_hooks.h"
#include "horizon_npc_sound_hook.h"
#include "horizon_guildcard_admission.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
namespace horizon_other {
namespace {
PreparedQuestMapping mapping;
bool attempted = false;
}
ModulePrepareResult prepare_horizon_other_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, CloneComposer shared) noexcept {
    ModulePrepareResult result{ModulePrepareStage::invalid_host, ExecutableIdentityResult::wrong_module,
                               MappingRedirectStatus::invalid_buffer, EquipmentQueueResult::invalid_host, nullptr};
    if (!image || !transaction.can_add_hooks()) return result;
    if (attempted) { result.stage = ModulePrepareStage::already_attempted; return result; }
    result.identity = certify_running_executable(image);
    if (result.identity != ExecutableIdentityResult::matched) {
        result.stage = ModulePrepareStage::identity_failed; return result;
    }
    if (!transaction.retain_failed_adds_for_coordinator()) return result;
    attempted = true;
    result.mapping = mapping.prepare(image).status;
    if (result.mapping != MappingRedirectStatus::prepared) {
        result.stage = ModulePrepareStage::mapping_failed; return result;
    }
    result.hooks = queue_quest_resource_hooks(transaction, image);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::resources_failed; return result;
    }
    result.hooks = queue_quest_classification_hook(transaction, image);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::classification_failed; return result;
    }
    result.hooks = queue_quest_open_hook(transaction, image, mapping);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::open_failed; return result;
    }
    result.hooks = queue_equipment_hooks(transaction, image, &verify_native_owner, shared);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::equipment_failed; return result;
    }
    result.hooks = queue_npc_sound_hook(transaction, image);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::npc_sound_failed; return result;
    }
    result.hooks = queue_guildcard_admission_hooks(transaction, image, mapping);
    if (result.hooks != EquipmentQueueResult::queued) {
        result.stage = ModulePrepareStage::guildcard_failed; return result;
    }
    result.stage = ModulePrepareStage::queued;
    result.prepared_mapping = &mapping;
    return result;
}
}
