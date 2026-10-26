#pragma once
#include "horizon_shield_status_session.h"
#include "horizon_shield_aux_lifecycles.h"
#include "horizon_shield_growth_calls.h"
#include "horizon_shield_loop_call.h"
#include "horizon_shield_sync_calls.h"
#include "horizon_shield_remote_call.h"
#include "horizon_shield_multiplier_call.h"
#include "horizon_shield_reaction_call.h"

namespace horizon_other {
struct ShieldEquipmentBindings {
    stormslinger::StormboltStatusUpdateBindings status{};
    stormslinger::StormboltTimerUpdateBindings timer{};
    stormslinger::StormboltPowerCellCompleteGrowthBindings growth{};
    stormslinger::StormboltLoopComponentUpdateBindings loop{};
    stormslinger::StormboltSyncPackBindings sync{};
    stormslinger::StormboltRemoteUpdateBindings remote{};
};
enum class ShieldEquipmentStage {
    invalid_state, prepared, activated, calls_restored, released,
    status_failed, auxiliary_failed, growth_failed, loop_failed, remote_failed,
    multiplier_failed, reaction_failed, sync_failed, frames_pending, restoration_pending
};

// Equipment-only composition. ONE host owns all stores and the entire hook
// transaction. It certifies native API/resource/codec bindings, installs before
// object birth, and provides real execution/writer quiescence and frame drain.
// No transaction commit/stop, bootstrap, live-object adoption or hot unloading.
template<std::size_t Capacity, std::size_t LoopCapacity>
class ShieldEquipmentSession {
    ShieldStatusSession<Capacity> status_;
    ShieldAuxiliaryLifecycles<Capacity,LoopCapacity> auxiliary_;
    ShieldGrowthCalls<Capacity,LoopCapacity> growth_;
    ShieldLoopCall<LoopCapacity,Capacity> loop_;
    ShieldSyncCalls<Capacity> sync_;
    ShieldRemoteCall<Capacity,LoopCapacity> remote_;
    ShieldMultiplierCall<Capacity> multiplier_;
    ShieldReactionCall<Capacity> reaction_;
    stormslinger::HookTransaction* transaction_=nullptr;
    bool coherent_, attempted_=false, prepared_=false, activation_attempted_=false;
    bool status_touched_=false, auxiliary_touched_=false, restored_=false, released_=false;
    ShieldEquipmentStage last_=ShieldEquipmentStage::invalid_state;
    static bool same_inputs(const stormslinger::StormboltLoopInputBindings& a,
        const stormslinger::StormboltLoopInputBindings& b) noexcept {
        return a.skill_vtable==b.skill_vtable && a.status_vtable==b.status_vtable &&
            a.resource_slot==b.resource_slot && a.resource_vtable==b.resource_vtable &&
            a.target_weapon_vtable==b.target_weapon_vtable;
    }
    static bool coherent(const ShieldEquipmentBindings& b) noexcept {
        const auto& i=b.status.inputs;
        const auto q=b.status.query;
        // Shield-Weaver is an equipment effect, independent of the held weapon.
        // A Stormslinger-only input binding would suppress the native skill path.
        return q && !i.target_weapon_vtable &&
            same_inputs(i,b.timer.inputs) && same_inputs(i,b.growth.growth.loop.inputs) &&
            same_inputs(i,b.loop.audio.inputs) && same_inputs(i,b.sync.inputs) &&
            same_inputs(i,b.remote.inputs) && same_inputs(i,b.remote.audio.inputs) &&
            q==b.timer.query && q==b.growth.growth.loop.query && q==b.loop.audio.query &&
            q==b.sync.query && q==b.remote.audio.query &&
            b.growth.master_vtable==b.sync.master_vtable &&
            b.growth.growth.manager_slot==b.remote.manager_slot &&
            b.growth.growth.local_player==b.remote.local_player &&
            b.growth.equipment==b.remote.equipment;
    }
public:
    ShieldEquipmentSession(const std::uint8_t* image,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        stormslinger::StormboltDamageExceptionStore<Capacity>& damages,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& events,
        const ShieldEquipmentBindings& b)
        : status_(image,statuses,b.status,b.timer), auxiliary_(image,damages,loops,events),
          growth_(image,statuses,damages,loops,events,b.growth), loop_(image,loops,statuses,b.loop),
          sync_(image,events,statuses,b.sync), remote_(image,statuses,loops,b.remote),
          multiplier_(image,statuses,b.status.inputs,b.status.query),
          reaction_(image,statuses,b.status.inputs,b.status.query),
          coherent_(image && coherent(b) && b.status.manager_slot==
              reinterpret_cast<std::uintptr_t>(image)+0x500ECA0) {}
    ShieldEquipmentSession(const ShieldEquipmentSession&)=delete;
    ShieldEquipmentSession& operator=(const ShieldEquipmentSession&)=delete;
    ShieldEquipmentStage last() const noexcept { return last_; }
    bool in_flight() const noexcept { return sync_.in_flight(); }
    bool restoration_pending() const noexcept {
        return status_.restoration_pending() || growth_.restoration_pending() ||
            loop_.restoration_pending() || sync_.restoration_pending() || remote_.restoration_pending() ||
            multiplier_.restoration_pending() || reaction_.restoration_pending();
    }
    ShieldEquipmentStage prepare_while_quiescent(stormslinger::HookTransaction& tx) {
        if (attempted_ || !coherent_ || !tx.can_add_hooks()) return last_=ShieldEquipmentStage::invalid_state;
        attempted_=true; transaction_=&tx;
        const auto s=status_.prepare_while_quiescent(tx);
        status_touched_=s!=ShieldStatusPrepare::invalid_state;
        if (s!=ShieldStatusPrepare::prepared) return last_=ShieldEquipmentStage::status_failed;
        const auto a=auxiliary_.prepare_while_quiescent(tx);
        auxiliary_touched_=a!=ShieldAuxPrepare::invalid_state;
        if (a!=ShieldAuxPrepare::prepared) return last_=ShieldEquipmentStage::auxiliary_failed;
        if (!growth_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::growth_failed;
        if (!loop_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::loop_failed;
        if (!remote_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::remote_failed;
        if (!multiplier_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::multiplier_failed;
        if (!reaction_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::reaction_failed;
        if (!sync_.prepare_while_quiescent(tx)) return last_=ShieldEquipmentStage::sync_failed;
        prepared_=true;
        return last_=ShieldEquipmentStage::prepared;
    }
    // Host commits its entire transaction while still quiescent, then calls
    // here. Any failure must use restore below BEFORE host transaction stop.
    ShieldEquipmentStage activate_after_shared_commit_while_quiescent() {
        if (!prepared_ || activation_attempted_ || restored_ || !transaction_ ||
            !transaction_->has_library_state() || !transaction_->last.active)
            return last_=ShieldEquipmentStage::invalid_state;
        activation_attempted_=true;
        if (!status_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::status_failed;
        if (!loop_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::loop_failed;
        if (!remote_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::remote_failed;
        if (!multiplier_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::multiplier_failed;
        if (!reaction_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::reaction_failed;
        if (!growth_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::growth_failed;
        // Producer last, after the paired receive/complete dependencies exist.
        if (!sync_.activate_after_shared_commit_while_quiescent()) return last_=ShieldEquipmentStage::sync_failed;
        return last_=ShieldEquipmentStage::activated;
    }
    // Prevent new producers and let pending native frames complete before
    // entering quiescence. Do not remove completion callbacks to force drain.
    ShieldEquipmentStage restore_calls_while_quiescent() {
        if (!attempted_ || released_) return last_=ShieldEquipmentStage::invalid_state;
        if (sync_.in_flight()) return last_=ShieldEquipmentStage::frames_pending;
        if (!sync_.restore_while_quiescent() || !growth_.restore_while_quiescent() ||
            !reaction_.restore_while_quiescent() || !multiplier_.restore_while_quiescent() ||
            !remote_.restore_while_quiescent() || !loop_.restore_while_quiescent() ||
            !status_.restore_updates_while_quiescent()) return last_=ShieldEquipmentStage::restoration_pending;
        restored_=true;
        return last_=ShieldEquipmentStage::calls_restored;
    }
    // Host has stopped the ENTIRE shared transaction and drained lifecycle
    // frames. Failed releases retain ownership and may be retried in place.
    ShieldEquipmentStage release_after_shared_stop() {
        if (!restored_ || released_ || !transaction_ || transaction_->has_library_state() || restoration_pending())
            return last_=ShieldEquipmentStage::invalid_state;
        if (!reaction_.release_after_shared_stop() || !multiplier_.release_after_shared_stop() ||
            !loop_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
        if (!sync_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
        if (!growth_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
        if (!remote_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
        if (auxiliary_touched_) {
            if (!auxiliary_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
            auxiliary_touched_=false;
        }
        if (status_touched_) {
            if (!status_.release_after_shared_stop()) return last_=ShieldEquipmentStage::restoration_pending;
            status_touched_=false;
        }
        released_=true;
        return last_=ShieldEquipmentStage::released;
    }
};
}
