#pragma once
#include "horizon_module_prepare.h"
#include "horizon_mapping_transaction.h"
#include "horizon_shield_equipment_session.h"
#include "horizon_tutorial_trigger.h"

namespace horizon_other {
enum class OtherActivationStage {
    invalid_state, prepared, calls_restored, activated, stopped, equipment_prepare_failed, mapping_failed,
    hooks_failed, equipment_activate_failed, rollback_pending,
    tutorial_prepare_failed, tutorial_activate_failed
};
struct OtherActivationResult {
    OtherActivationStage stage=OtherActivationStage::invalid_state;
    ShieldEquipmentStage equipment=ShieldEquipmentStage::invalid_state;
    ShieldEquipmentStage restore=ShieldEquipmentStage::invalid_state;
    MappingInstallResult mapping=MappingInstallResult::invalid_state;
    MH_STATUS hooks=MH_OK, rollback=MH_OK;
};

// Only the owner of the ENTIRE shared transaction may use this coordinator.
// Host certifies native bindings, begins/queues the module's original hooks via
// prepare_horizon_other_hooks, and keeps execution/writers quiescent and native
// frames drained throughout activation/stop/retry. Install before object birth.
// Owns neither stores nor a new transaction; never performs thread suspension,
// a game launch, DLL unload or an automatic destructor rollback.
template<std::size_t Capacity, std::size_t LoopCapacity>
class OtherModuleActivation {
    const std::uint8_t* image_;
    ShieldEquipmentSession<Capacity,LoopCapacity> equipment_;
    QuestMappingTransaction mapping_;
    TutorialTriggerCall tutorial_;
    const PreparedQuestMapping* borrowed_mapping_=nullptr;
    stormslinger::HookTransaction* transaction_=nullptr;
    bool attempted_=false, equipment_touched_=false, terminal_=false;
    bool prepared_=false, calls_restored_=false;
    OtherActivationResult last_{};
    OtherActivationResult rollback(OtherActivationStage reason) {
        // A pending sync frame prevents ALL instruction mutation. In particular
        // do not stop its completion/lifecycle hooks or remove quest mapping.
        if (restore_before_shared_stop_while_quiescent().stage!=OtherActivationStage::calls_restored)
            return last_;
        last_.rollback=transaction_->stop();
        if (last_.rollback!=MH_OK) {
            last_.stage=OtherActivationStage::rollback_pending;
            return last_;
        }
        if (release_after_shared_stop_while_quiescent().stage!=OtherActivationStage::stopped)
            return last_;
        last_.stage=reason;
        return last_;
    }
public:
    // Combined hosts restore ALL modules' direct patches before stopping the
    // shared transaction once. These two phases never stop/commit that transaction.
    OtherActivationResult restore_before_shared_stop_while_quiescent() {
        if (!attempted_ || terminal_) return {};
        if (equipment_touched_ && !calls_restored_) {
            last_.restore=equipment_.restore_calls_while_quiescent();
            if (last_.restore!=ShieldEquipmentStage::calls_restored) {
                last_.stage=OtherActivationStage::rollback_pending;
                return last_;
            }
        }
        if (!tutorial_.restore_while_quiescent()) {
            last_.stage=OtherActivationStage::rollback_pending;
            return last_;
        }
        calls_restored_=true;
        last_.stage=OtherActivationStage::calls_restored;
        return last_;
    }
    OtherActivationResult release_after_shared_stop_while_quiescent() {
        if (!attempted_ || terminal_ || !calls_restored_ || !transaction_ ||
            transaction_->has_library_state()) return {};
        if (!mapping_.restore_while_quiescent()) {
            last_.stage=OtherActivationStage::rollback_pending;
            return last_;
        }
        if (!tutorial_.release_after_shared_stop_while_quiescent()) {
            last_.stage=OtherActivationStage::rollback_pending;
            return last_;
        }
        if (equipment_touched_) {
            last_.restore=equipment_.release_after_shared_stop();
            if (last_.restore!=ShieldEquipmentStage::released) {
                last_.stage=OtherActivationStage::rollback_pending;
                return last_;
            }
            equipment_touched_=false;
        }
        terminal_=true;
        last_.stage=OtherActivationStage::stopped;
        return last_;
    }
public:
    OtherModuleActivation(const std::uint8_t* image,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        stormslinger::StormboltDamageExceptionStore<Capacity>& damages,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& events,
        const ShieldEquipmentBindings& bindings)
        : image_(image),equipment_(image,statuses,damages,loops,events,bindings) {}
    OtherModuleActivation(const OtherModuleActivation&)=delete;
    OtherModuleActivation& operator=(const OtherModuleActivation&)=delete;
    OtherActivationResult last() const noexcept { return last_; }
    bool in_flight() const noexcept { return equipment_.in_flight(); }
    // Failure retains ownership. Combined host must run both restore phases,
    // alongside its other modules, rather than calling the single-owner stop.
    OtherActivationResult prepare_while_quiescent(stormslinger::HookTransaction& tx,
        const ModulePrepareResult& prepared) {
        if (attempted_ || !image_ || prepared.stage!=ModulePrepareStage::queued ||
            !prepared.prepared_mapping || prepared.prepared_mapping->image()!=image_ ||
            !tx.can_add_hooks()) return {};
        attempted_=true;transaction_=&tx;
        last_.equipment=equipment_.prepare_while_quiescent(tx);
        equipment_touched_=last_.equipment!=ShieldEquipmentStage::invalid_state;
        if (last_.equipment!=ShieldEquipmentStage::prepared) {
            last_.stage=OtherActivationStage::equipment_prepare_failed;
            return last_;
        }
        if (!tutorial_.prepare_while_quiescent(image_,tx)) {
            last_.stage=OtherActivationStage::tutorial_prepare_failed;
            return last_;
        }
        borrowed_mapping_=prepared.mapping_owned_by_shared_host ? prepared.prepared_mapping : nullptr;
        // In a combined build only the tutorial-menu branch is ours. The root
        // owns all quest redirects, including their rollback after shared stop.
        last_.mapping=mapping_.install_while_quiescent(
            const_cast<std::uint8_t*>(image_),*prepared.prepared_mapping,!borrowed_mapping_);
        if (last_.mapping!=MappingInstallResult::installed) {
            last_.stage=OtherActivationStage::mapping_failed;
            return last_;
        }
        prepared_=true;
        last_.stage=OtherActivationStage::prepared;
        return last_;
    }
    OtherActivationResult activate_after_shared_commit_while_quiescent() {
        if (!prepared_ || calls_restored_ || terminal_ ||
            last_.stage!=OtherActivationStage::prepared || !transaction_ ||
            !transaction_->has_library_state() || !transaction_->last.active) return {};
        if (borrowed_mapping_ && !quest_mapping_references_active(*borrowed_mapping_)) {
            last_.stage=OtherActivationStage::mapping_failed;return last_;
        }
        if (!tutorial_.activate_after_shared_commit_while_quiescent()) {
            last_.stage=OtherActivationStage::tutorial_activate_failed;
            return last_;
        }
        last_.equipment=equipment_.activate_after_shared_commit_while_quiescent();
        last_.stage=last_.equipment==ShieldEquipmentStage::activated
            ? OtherActivationStage::activated : OtherActivationStage::equipment_activate_failed;
        return last_;
    }
    OtherActivationResult activate_while_quiescent(stormslinger::HookTransaction& tx,
        const ModulePrepareResult& prepared) {
        const auto staged=prepare_while_quiescent(tx,prepared);
        if (staged.stage==OtherActivationStage::invalid_state) return staged;
        if (staged.stage!=OtherActivationStage::prepared) return rollback(staged.stage);
        last_.hooks=tx.commit_pending_without_rollback();
        if (last_.hooks!=MH_OK) return rollback(OtherActivationStage::hooks_failed);
        if (activate_after_shared_commit_while_quiescent().stage!=OtherActivationStage::activated)
            return rollback(last_.stage);
        last_.stage=OtherActivationStage::activated;
        return last_;
    }
    // Host first prevents new producer entries, lets pending completion frames
    // finish, then establishes true quiescence/drain. A pending result retains
    // the same coordinator/transaction/stores/relays for an explicit retry.
    OtherActivationResult stop_while_quiescent() {
        if (!attempted_ || !transaction_ || terminal_) return last_;
        return rollback(OtherActivationStage::stopped);
    }
};
}
