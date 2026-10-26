#pragma once
#include "horizon_executable_identity.h"
#include "horizon_quest_open_hook.h"
namespace horizon_other {
enum class ModulePrepareStage { queued, invalid_host, already_attempted, identity_failed,
                                mapping_failed, resources_failed, classification_failed,
                                open_failed, equipment_failed, npc_sound_failed, guildcard_failed };
struct ModulePrepareResult {
    ModulePrepareStage stage;
    ExecutableIdentityResult identity;
    MappingRedirectStatus mapping;
    EquipmentQueueResult hooks;
    const PreparedQuestMapping* prepared_mapping;
};
// Serialized, once-per-process preparation before resource/equipment population.
// Host owns the shared transaction. No begin/commit, mapping writes or installation
// permission is performed here. On failure the host must stop the entire transaction.
// DLL, clones, mapping and callback state must remain resident for process lifetime.
ModulePrepareResult prepare_horizon_other_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image, CloneComposer shared_bow_edits = nullptr) noexcept;
}
