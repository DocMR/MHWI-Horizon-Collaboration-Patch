#pragma once
#include "horizon_other_activation.h"
#include "horizon_shield_native_bindings.h"
#include <optional>

namespace horizon_other {
enum class OtherHostStage {
    idle, prepared, calls_restored, active, stopped, invalid_transaction, begin_failed,
    prepare_failed, bindings_failed, activation_failed, rollback_pending
};
struct OtherHostResult {
    OtherHostStage stage=OtherHostStage::idle;
    MH_STATUS transaction=MH_OK;
    ModulePrepareResult preparation{};
    OtherActivationResult activation{};
};

// Process-lifetime module host. Staged methods borrow the root transaction;
// only the single-owner convenience methods may commit/stop the ENTIRE transaction.
// The loader must call at an authenticated early boundary, before object birth,
// with native execution/writers quiescent. This class cannot create that boundary.
// Stores/transaction are borrowed so the final combined host can use one set.
template<std::size_t Capacity,std::size_t LoopCapacity>
class OtherModuleHost {
    stormslinger::HookTransaction& tx_;
    stormslinger::StormboltPowerCellStore<Capacity>& statuses_;
    stormslinger::StormboltDamageExceptionStore<Capacity>& damages_;
    stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops_;
    stormslinger::StormboltPowerCellSyncStore<Capacity>& events_;
    std::optional<OtherModuleActivation<Capacity,LoopCapacity>> activation_;
    bool attempted_=false;
    bool preparation_entered_=false;
    OtherHostResult last_{};

    OtherHostResult stop_preparation(OtherHostStage reason) {
        last_.transaction=tx_.stop();
        last_.stage=last_.transaction==MH_OK ? reason : OtherHostStage::rollback_pending;
        return last_;
    }
public:
    OtherModuleHost(stormslinger::HookTransaction& tx,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        stormslinger::StormboltDamageExceptionStore<Capacity>& damages,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& events)
        :tx_(tx),statuses_(statuses),damages_(damages),loops_(loops),events_(events) {}
    OtherModuleHost(const OtherModuleHost&)=delete;
    OtherModuleHost& operator=(const OtherModuleHost&)=delete;
    OtherHostResult last() const noexcept { return last_; }
    bool in_flight() const noexcept { return activation_ && activation_->in_flight(); }

    // Combined root has already begun the one shared transaction. This method
    // never begins, commits or stops it, including on partial preparation failure.
    OtherHostResult prepare_before_object_birth_while_quiescent(
        const std::uint8_t* image,std::size_t image_bytes,CloneComposer shared_bow=nullptr,
        const PreparedQuestMapping* shared_mapping=nullptr) {
        if (attempted_) return last_;
        attempted_=true;
        if (!image || image_bytes!=0x55D3000 || !tx_.can_add_hooks()) {
            last_.stage=OtherHostStage::invalid_transaction;
            return last_;
        }
        preparation_entered_=true;
        last_.preparation=prepare_horizon_other_hooks(tx_,image,shared_bow,shared_mapping);
        if (last_.preparation.stage!=ModulePrepareStage::queued) {
            last_.stage=OtherHostStage::prepare_failed;return last_;
        }
        ShieldEquipmentBindings bindings{};
        if (!bind_shield_equipment_endpoints(image,image_bytes,bindings)) {
            last_.stage=OtherHostStage::bindings_failed;return last_;
        }
        activation_.emplace(image,statuses_,damages_,loops_,events_,bindings);
        last_.activation=activation_->prepare_while_quiescent(tx_,last_.preparation);
        last_.stage=last_.activation.stage==OtherActivationStage::prepared
            ? OtherHostStage::prepared : OtherHostStage::activation_failed;
        return last_;
    }
    OtherHostResult activate_after_shared_commit_while_quiescent() {
        if (last_.stage!=OtherHostStage::prepared || !activation_) return last_;
        last_.activation=activation_->activate_after_shared_commit_while_quiescent();
        last_.stage=last_.activation.stage==OtherActivationStage::activated
            ? OtherHostStage::active : OtherHostStage::activation_failed;
        return last_;
    }
    OtherHostResult restore_before_shared_stop_while_quiescent() {
        if (!attempted_ || last_.stage==OtherHostStage::stopped) return last_;
        if (activation_) {
            last_.activation=activation_->restore_before_shared_stop_while_quiescent();
            if (last_.activation.stage!=OtherActivationStage::calls_restored) {
                last_.stage=OtherHostStage::rollback_pending;return last_;
            }
        }
        last_.stage=OtherHostStage::calls_restored;return last_;
    }
    OtherHostResult release_after_shared_stop_while_quiescent() {
        if (last_.stage!=OtherHostStage::calls_restored || tx_.has_library_state()) return last_;
        if (activation_) {
            last_.activation=activation_->release_after_shared_stop_while_quiescent();
            if (last_.activation.stage!=OtherActivationStage::stopped) {
                last_.stage=OtherHostStage::rollback_pending;return last_;
            }
        }
        last_.stage=OtherHostStage::stopped;return last_;
    }
    // Convenience for a host owning the ENTIRE transaction, never for one
    // participant in a combined startup. Keep the staged methods above instead.
    OtherHostResult start_before_object_birth_while_quiescent(
        const std::uint8_t* image,std::size_t image_bytes,CloneComposer shared_bow=nullptr,
        const PreparedQuestMapping* shared_mapping=nullptr) {
        if (attempted_) return last_;
        if (!image || image_bytes!=0x55D3000 ||
            (tx_.has_library_state() && !tx_.can_add_hooks())) {
            attempted_=true;last_.stage=OtherHostStage::invalid_transaction;return last_;
        }
        if (!tx_.has_library_state()) {
            last_.transaction=tx_.begin();
            if (last_.transaction!=MH_OK) {
                attempted_=true;last_.stage=OtherHostStage::begin_failed;return last_;
            }
        }
        const auto prepared=prepare_before_object_birth_while_quiescent(image,image_bytes,shared_bow,shared_mapping);
        if (prepared.stage!=OtherHostStage::prepared) {
            if (activation_) {
                last_.activation=activation_->stop_while_quiescent();
                if (last_.activation.stage==OtherActivationStage::rollback_pending)
                    last_.stage=OtherHostStage::rollback_pending;
                return last_;
            }
            return stop_preparation(prepared.stage);
        }
        last_.transaction=tx_.commit_pending_without_rollback();
        if (last_.transaction==MH_OK &&
            activate_after_shared_commit_while_quiescent().stage==OtherHostStage::active) return last_;
        last_.activation=activation_->stop_while_quiescent();
        last_.stage=last_.activation.stage==OtherActivationStage::rollback_pending
            ? OtherHostStage::rollback_pending : OtherHostStage::activation_failed;
        return last_;
    }

    // Caller prevents new producers, drains frames and establishes quiescence.
    // Keep this same host, stores, transaction and DLL alive on pending rollback.
    OtherHostResult stop_while_quiescent() {
        if (!attempted_ || last_.stage==OtherHostStage::stopped) return last_;
        if (activation_) {
            last_.activation=activation_->stop_while_quiescent();
            last_.stage=last_.activation.stage==OtherActivationStage::rollback_pending
                ? OtherHostStage::rollback_pending : OtherHostStage::stopped;
        } else if (preparation_entered_) {
            // Preparation can leave queued hooks before activation_ exists.
            // The single-owner stop must also clean up that partial state.
            return stop_preparation(OtherHostStage::stopped);
        }
        return last_;
    }
};
}
