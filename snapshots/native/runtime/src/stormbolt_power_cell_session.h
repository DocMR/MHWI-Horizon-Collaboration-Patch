#pragma once
#include "stormbolt_native_bindings.h"
#include "stormbolt_status_lifecycle_hooks.h"
#include "stormbolt_loop_audio_dispatch.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include "stormbolt_owned_call_site.h"
#include "stormbolt_damage_exception_hooks.h"
#include "stormbolt_remote_update_dispatch.h"
#include "stormbolt_power_cell_sync_store.h"
#include "stormbolt_sync_producer_session.h"
#include "stormbolt_growth_session.h"
#include "stormbolt_class48_skill_hook.h"
#include "stormbolt_elemental_class48_patch.h"
#include "stormbolt_recoil_ammo40.h"
#include "stormbolt_self_damage_shot.h"
#include <optional>

extern "C" void stormbolt_status_update_span_entry();
extern "C" void stormbolt_loop_update_span_entry();
extern "C" void stormbolt_remote_update_span_entry();
namespace stormslinger {
// Participates in the caller's already-open shared transaction. The caller
// authenticates executable/resources and keeps all native execution quiescent.
// Restore these CALLs before stopping the shared transaction; release bridges
// afterwards. This does not independently commit/stop other shooting hooks.
template<std::size_t L,std::size_t S>
class StormboltPowerCellSession {
    StormboltPowerCellStore<S> statuses_;
    StormboltPowerCellLoopStore<L> loops_;
    StormboltDamageExceptionStore<S> damage_states_;
    StormboltPowerCellSyncStore<S> sync_events_;
    StormboltLifecycleHooks<S,StormboltPowerCellSyncEventState,StormboltPowerCellSyncToken> sync_lifecycle_{sync_events_};
    const StormboltNativeBindings native_;
    StormboltStatusLifecycleHooks<S> status_lifecycle_;
    StormboltClass48SkillHook<S> class48_hook_;
    StormboltLifecycleHooks<S,StormboltDamageExceptionState,StormboltDamageExceptionToken> damage_lifecycle_{damage_states_};
    StormboltDamageExceptionHooks<S> damage_hooks_;
    StormboltLifecycleHooks<L,StormboltPowerCellLoopState,StormboltPowerCellLoopToken> loop_lifecycle_{loops_};
    StormboltStatusUpdateDispatch<S> status_dispatch_;
    StormboltTimerUpdateDispatch<S> timer_dispatch_;
    StormboltLoopComponentDispatch<L,S> loop_dispatch_;
    StormboltRemoteUpdateDispatch<S,L> remote_dispatch_;
    const bool remote_enabled_;
    StormboltSyncProducerSession<S> producer_;
    const bool producer_enabled_;
    std::optional<StormboltGrowthSession<S,L>> growth_;
    bool remote_published_=false;
    std::optional<StormboltOwnedReplayCallSite<6>> remote_call_;
    std::optional<StormboltOwnedReplayCallSite<7>> status_call_,loop_call_;
    std::optional<StormboltOwnedCallSite> timer_call_;
    std::optional<StormboltElementalClass48Patch> elemental_class48_;
    std::optional<StormboltRecoilAmmo40Patch> recoil_ammo40_;
    HookTransaction* transaction_=nullptr;
    std::array<unsigned char,7> status_expected_{};
    std::array<unsigned char,7> timer_expected_{};
    bool timer_published_=false;
    bool started_=false,prepared_=false,status_published_=false,loop_published_=false;
public:
    explicit StormboltPowerCellSession(const StormboltNativeBindings& native,
        const StormboltRemoteUpdateBindings& remote={},
        const StormboltSyncPackBindings& producer={},
        const std::array<std::uintptr_t,3>& growth_sites={},
        const StormboltPowerCellCompleteGrowthBindings& growth={})
        :native_(native),status_lifecycle_(statuses_,native.power_cell.status_vtable),
         class48_hook_(statuses_,stormbolt_class48_skill_bindings(native)),
         damage_hooks_(damage_states_,native.damage_vtable),
         status_dispatch_(statuses_,stormbolt_status_update_bindings(native)),
         timer_dispatch_(statuses_,stormbolt_timer_update_bindings(native)),
         loop_dispatch_(loops_,statuses_,stormbolt_loop_component_bindings(native)),
         remote_dispatch_(statuses_,loops_,remote),remote_enabled_(remote.decode!=nullptr),
         producer_(native.power_cell_sync_pack_site,native.power_cell_sync_complete_site,sync_events_,statuses_,producer),
         producer_enabled_(producer.encode!=nullptr) {
            if(growth_sites[0] || growth_sites[1] || growth_sites[2])
                growth_.emplace(growth_sites,statuses_,damage_states_,loops_,sync_events_,growth);
         }
    StormboltPowerCellSession(const StormboltPowerCellSession&)=delete;
    StormboltPowerCellSession& operator=(const StormboltPowerCellSession&)=delete;
    auto& statuses() noexcept {return statuses_;}
    auto& loops() noexcept {return loops_;}
    auto& damage_states() noexcept {return damage_states_;}
    auto& sync_events() noexcept {return sync_events_;}
    bool shot_self_damage(void* weapon,std::uint32_t snapshot) {
        const StormboltSelfDamageShotBindings api{native_.coefficients.parameter_vtable,
            native_.damage_vtable,native_.health_manager_vtable,
            stormbolt_loop_component_bindings(native_).audio,native_.health_adjustment,
            native_.skill_prompt,native_.equipment_effect};
        return apply_stormbolt_shot_self_damage(weapon,snapshot,statuses_,loops_,api);
    }
    bool prepare(HookTransaction& transaction,const std::array<unsigned char,7>& status_expected,
        const std::array<unsigned char,7>& timer_expected={0xE8,0x83,0xA9,0xD9,0,0x84,0xC0}) {
        if (started_ || !transaction.can_add_hooks() || !native_.power_cell_status_update_site || !native_.loop_update_site || !native_.power_cell_timer_update_site) return false;
        if (remote_enabled_ && !native_.power_cell_remote_update_site) return false;
        const bool sync_enabled=native_.power_cell_sync_construct || native_.power_cell_sync_destroy || native_.power_cell_sync_vtable;
        if (sync_enabled && (!native_.power_cell_sync_construct || !native_.power_cell_sync_destroy || !native_.power_cell_sync_vtable)) return false;
        if (producer_enabled_ && (!sync_enabled || !native_.power_cell_sync_pack_site || !native_.power_cell_sync_complete_site)) return false;
        started_=true;transaction_=&transaction;status_expected_=status_expected;timer_expected_=timer_expected;
#if !defined(STORMSLINGER_DIAGNOSTIC_STATUS_LIFECYCLE_ONLY)
        if (native_.elemental_class48_site) {
            elemental_class48_.emplace(&transaction);
            if (!elemental_class48_->prepare(reinterpret_cast<void*>(native_.elemental_class48_site),
                reinterpret_cast<void*>(native_.elemental_ammo_class48_site))) return false;
        }
        if (native_.recoil_ammo40_site) {
            recoil_ammo40_.emplace(&transaction);
            if(!recoil_ammo40_->prepare(reinterpret_cast<void*>(native_.recoil_ammo40_site))) return false;
        }
        if (native_.class48_skill_multiplier && class48_hook_.add_to_transaction(transaction,reinterpret_cast<void*>(native_.class48_skill_multiplier))!=MH_OK) return false;
        if (sync_enabled && sync_lifecycle_.add_constructor_to_transaction(transaction,
            reinterpret_cast<void*>(native_.power_cell_sync_construct),
            reinterpret_cast<void*>(native_.power_cell_sync_destroy))!=MH_OK) return false;
        if (damage_lifecycle_.add_constructor_to_transaction(transaction,reinterpret_cast<void*>(native_.damage_construct),
            reinterpret_cast<void*>(native_.damage_destroy))!=MH_OK) return false;
        if (damage_hooks_.add_to_transaction(transaction,reinterpret_cast<void*>(native_.damage_update),
            reinterpret_cast<void*>(native_.damage_absorb))!=MH_OK) return false;
#endif
        if (status_lifecycle_.add_to_transaction(transaction,reinterpret_cast<void*>(native_.power_cell.construct),
            reinterpret_cast<void*>(native_.power_cell.reset),reinterpret_cast<void*>(native_.power_cell.destroy))!=MH_OK) return false;
#if !defined(STORMSLINGER_DIAGNOSTIC_STATUS_LIFECYCLE_ONLY)
        if (loop_lifecycle_.add_constructor_to_transaction(transaction,reinterpret_cast<void*>(native_.loop_construct),
            reinterpret_cast<void*>(native_.loop_destroy))!=MH_OK) return false;
#endif
#if defined(STORMSLINGER_DIAGNOSTIC_NO_POWER_CELL_CALLS)
        prepared_=true;return true;
#else
        if (!status_dispatch_.publish()) return false;
        status_published_=true;
        if (!loop_dispatch_.publish()) return false;
        loop_published_=true;
        if (!timer_dispatch_.publish()) return false;
        timer_published_=true;
        status_call_.emplace(&transaction);loop_call_.emplace(&transaction);
        if(!status_call_->prepare(reinterpret_cast<void*>(native_.power_cell_status_update_site),
            reinterpret_cast<std::uintptr_t>(&stormbolt_status_update_span_entry),status_expected_)) return false;
        if(!loop_call_->prepare(reinterpret_cast<void*>(native_.loop_update_site),
            reinterpret_cast<std::uintptr_t>(&stormbolt_loop_update_span_entry),std::array<unsigned char,7>{0x80,0xBF,0xB8,0x09,0,0,0})) return false;
        // Keep native TEST AL,AL at site+5 intact for both old CALL returns
        // and the new entry. Its repeated TEST has the same flags/result.
        std::array<unsigned char,7> timer_live{};SIZE_T timer_count=0;
        if(timer_expected_[5]!=0x84 || timer_expected_[6]!=0xC0 ||
            !ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(native_.power_cell_timer_update_site),
                timer_live.data(),timer_live.size(),&timer_count) || timer_count!=timer_live.size() || timer_live!=timer_expected_) return false;
        std::array<unsigned char,5> timer_original{};
        std::copy_n(timer_expected_.begin(),5,timer_original.begin());
        timer_call_.emplace(&transaction);
        if(!timer_call_->prepare(reinterpret_cast<void*>(native_.power_cell_timer_update_site),
            reinterpret_cast<std::uintptr_t>(&stormbolt_timer_update_entry),timer_original)) return false;
        if (remote_enabled_) {
            if(!remote_dispatch_.publish()) return false;
            remote_published_=true;
            remote_call_.emplace(&transaction);
            if(!remote_call_->prepare(reinterpret_cast<void*>(native_.power_cell_remote_update_site),
                reinterpret_cast<std::uintptr_t>(&stormbolt_remote_update_span_entry),std::array<unsigned char,6>{0x8B,0x87,0x98,0x59,0,0})) return false;
            remote_published_=true;
        }
        if (producer_enabled_ && !producer_.prepare(&transaction)) return false;
        if (growth_ && !growth_->prepare(&transaction)) return false;
        prepared_=true;return true;
#endif
    }
    bool activate_after_shared_commit() {
        if (!prepared_ || !transaction_ || !transaction_->last.active) return false;
        if (elemental_class48_ && !elemental_class48_->activate()) return false;
        if (recoil_ammo40_ && !recoil_ammo40_->activate()) return false;
#if defined(STORMSLINGER_DIAGNOSTIC_NO_POWER_CELL_CALLS)
        return true; // Only the native hooks queued above participate.
#else
        if (!status_call_ || !status_call_->activate() || !loop_call_ || !loop_call_->activate()) return false;
        if (!timer_call_ || !timer_call_->activate()) return false;
        if (producer_enabled_ && !producer_.activate()) return false;
        if (growth_ && !growth_->activate()) return false;
        return !remote_enabled_ || (remote_call_ && remote_call_->activate());
#endif
    }
    bool restore_update_patches() {
        if (recoil_ammo40_ && !recoil_ammo40_->restore()) return false;
        if (elemental_class48_ && !elemental_class48_->restore()) return false;
        if (growth_ && !growth_->restore_patches()) return false;
        return (!remote_call_ || remote_call_->restore()) && producer_.restore_patches() && (!timer_call_ || timer_call_->restore()) && (!loop_call_ || loop_call_->restore()) && (!status_call_ || status_call_->restore());
    }
    // The host drains native callbacks before withdrawing their dispatch owners.
    // Keep this phase separate from instruction writes at a frozen boundary.
    bool withdraw_updates_after_quiescence() {
        if (recoil_ammo40_ && recoil_ammo40_->active()) return false;
        if (elemental_class48_ && elemental_class48_->active()) return false;
        if((remote_call_ && remote_call_->active()) || (timer_call_ && timer_call_->active()) || (loop_call_ && loop_call_->active()) || (status_call_ && status_call_->active())) return false;
        if(growth_ && !growth_->withdraw_after_quiescence()) return false;
        if(!producer_.withdraw_after_quiescence()) return false;
        if (remote_published_) {if (!remote_dispatch_.withdraw()) return false;remote_published_=false;}
        if (timer_published_) {if (!timer_dispatch_.withdraw()) return false;timer_published_=false;}
        if (loop_published_) {if (!loop_dispatch_.withdraw()) return false;loop_published_=false;}
        if (status_published_) {if (!status_dispatch_.withdraw()) return false;status_published_=false;}
        prepared_=false;return true;
    }
    bool restore_updates() {return restore_update_patches() && withdraw_updates_after_quiescence();}
    bool release_after_shared_stop() {
        if (recoil_ammo40_ && recoil_ammo40_->active()) return false;
        if (elemental_class48_ && elemental_class48_->active()) return false;
        if (growth_ && growth_->active()) return false;
        if (producer_.active() || !transaction_ || transaction_->has_library_state() || (status_call_ && status_call_->active()) || (loop_call_ && loop_call_->active()) ||
            (timer_call_ && timer_call_->active()) || (remote_call_ && remote_call_->active()) || remote_published_ || timer_published_ || status_published_ || loop_published_) return false;
        if(growth_ && !growth_->release_after_shared_stop()) return false;
        if(producer_enabled_ && !producer_.release_after_shared_stop()) return false;
        if (elemental_class48_ && !elemental_class48_->release_after_quiescence()) return false;
        if (recoil_ammo40_ && !recoil_ammo40_->release_after_quiescence()) return false;
        if(timer_call_ && !timer_call_->release_after_quiescence()) return false;
        if(status_call_ && !status_call_->release_after_quiescence()) return false;
        if(loop_call_ && !loop_call_->release_after_quiescence()) return false;
        if(remote_call_ && !remote_call_->release_after_quiescence()) return false;
        if (loop_lifecycle_.owns_bridge() && !loop_lifecycle_.release_after_unhook()) return false;
        if (status_lifecycle_.owns_bridge() && !status_lifecycle_.release_after_unhook()) return false;
        if (damage_hooks_.owns_bridge() && !damage_hooks_.release_after_unhook()) return false;
        if (damage_lifecycle_.owns_bridge() && !damage_lifecycle_.release_after_unhook()) return false;
        if (class48_hook_.owns_bridge() && !class48_hook_.release_after_unhook()) return false;
        if (sync_lifecycle_.owns_bridge() && !sync_lifecycle_.release_after_unhook()) return false;
        return true;
    }
};
} // namespace stormslinger

