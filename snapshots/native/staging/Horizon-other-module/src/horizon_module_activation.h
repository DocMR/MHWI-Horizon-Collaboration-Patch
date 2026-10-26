#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
#include "horizon_module_prepare.h"
#include "horizon_mapping_transaction.h"
#include "horizon_tutorial_trigger.h"
namespace horizon_other {
enum class ModuleActivationStage { activated, stopped, invalid_state, mapping_failed,
                                   hook_failed, rollback_pending, tutorial_prepare_failed,
                                   tutorial_activate_failed };
struct ModuleActivationResult {
    ModuleActivationStage stage;
    MappingInstallResult mapping;
    MH_STATUS hooks;
    MH_STATUS rollback;
};
// Only the owner of the ENTIRE shared hook transaction may use this coordinator.
// Caller must serialize and keep target execution/other writers quiescent across
// each call. No thread suspension, bootstrap, unload or installation permission.
// Retain this object, transaction, DLL, mapping and clones for process lifetime.
class ModuleActivation {
    QuestMappingTransaction mapping_;
    TutorialTriggerCall tutorial_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    bool attempted_ = false;
    ModuleActivationResult result_{ModuleActivationStage::invalid_state,
        MappingInstallResult::invalid_state, MH_OK, MH_OK};
    ModuleActivationResult rollback(ModuleActivationStage failure) noexcept;
public:
    ModuleActivation() = default;
    ModuleActivation(const ModuleActivation&) = delete;
    ModuleActivation& operator=(const ModuleActivation&) = delete;
    ModuleActivationResult activate_while_quiescent(
        stormslinger::HookTransaction& transaction, std::uint8_t* image,
        const ModulePrepareResult& prepared) noexcept;
    ModuleActivationResult stop_while_quiescent() noexcept;
};
}
