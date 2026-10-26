#include "horizon_module_activation.h"
namespace horizon_other {
ModuleActivationResult ModuleActivation::rollback(ModuleActivationStage failure) noexcept {
    if (!tutorial_.restore_while_quiescent()) {
        result_.stage = ModuleActivationStage::rollback_pending;
        return result_;
    }
    // A failed commit can leave a subset enabled. Never remove mapping first.
    result_.rollback = transaction_->stop();
    if (result_.rollback != MH_OK) {
        result_.stage = ModuleActivationStage::rollback_pending;
        return result_;
    }
    if (!mapping_.restore_while_quiescent()) {
        result_.stage = ModuleActivationStage::rollback_pending;
        return result_;
    }
    if (!tutorial_.release_after_shared_stop_while_quiescent()) {
        result_.stage = ModuleActivationStage::rollback_pending;
        return result_;
    }
    result_.stage = failure;
    return result_;
}
ModuleActivationResult ModuleActivation::activate_while_quiescent(
    stormslinger::HookTransaction& transaction, std::uint8_t* image,
    const ModulePrepareResult& prepared) noexcept {
    if (attempted_ || !image || prepared.stage != ModulePrepareStage::queued ||
        !prepared.prepared_mapping || prepared.prepared_mapping->image() != image ||
        !transaction.can_add_hooks()) {
        return {ModuleActivationStage::invalid_state, MappingInstallResult::invalid_state,
                MH_OK, MH_OK};
    }
    attempted_ = true;
    transaction_ = &transaction;
    if (!tutorial_.prepare_while_quiescent(image,transaction))
        return rollback(ModuleActivationStage::tutorial_prepare_failed);
    result_.mapping = mapping_.install_while_quiescent(image, *prepared.prepared_mapping);
    if (result_.mapping != MappingInstallResult::installed)
        return rollback(ModuleActivationStage::mapping_failed);
    result_.hooks = transaction.commit_pending_without_rollback();
    if (result_.hooks != MH_OK)
        return rollback(ModuleActivationStage::hook_failed);
    if (!tutorial_.activate_after_shared_commit_while_quiescent())
        return rollback(ModuleActivationStage::tutorial_activate_failed);
    result_.stage = ModuleActivationStage::activated;
    return result_;
}
ModuleActivationResult ModuleActivation::stop_while_quiescent() noexcept {
    if (!attempted_ || !transaction_) return result_;
    return rollback(ModuleActivationStage::stopped);
}
}
