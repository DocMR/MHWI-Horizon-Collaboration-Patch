#pragma once
#include "stormbolt_native_bindings.h"
#include "stormbolt_special_action_session.h"
#include "stormbolt_gatling_start_session.h"
#include "stormbolt_middle_session.h"
#include "stormbolt_last_session.h"
#include "stormbolt_power_cell_session.h"
#include "stormbolt_evade_reload_effect_session.h"
#include "stormbolt_charge_hud_hook.h"
#include "stormbolt_operation_hint_hook.h"
#include "stormbolt_itemwindow_hook.h"
#include "stormbolt_ordinary_reticle_hook.h"
#include "stormbolt_motion_event_hook.h"
#include "stormbolt_muzzle_effect_hook.h"
#include "stormbolt_swap_energy_hooks.h"
#include "stormbolt_itemwindow_mode.h"
#include <optional>

namespace stormslinger {
enum class StormboltSessionState { fresh, prepared, enabled, stopped, recovery_required };
enum class StormboltSessionIssue { none, wrong_state, invalid_bindings, lifecycle_hooks,
    shell_hook, coefficient_pointer, action_pointer, activation, action_pointer_restore,
    pointer_restore, hook_restore, special_action_pointer, special_action_restore, start_pointer, start_restore,
    middle_pointer, middle_restore, last_pointer, last_restore, update_pointer, update_restore,
    power_cell_prepare, power_cell_activation, power_cell_restore,
    source_transition_prepare, source_transition_activation, source_transition_restore,
    source_condition_prepare, source_condition_activation, source_condition_restore,
    source_manager_prepare, source_manager_restore,
    evade_prepare, evade_activation, evade_restore, hud_prepare, hud_restore, motion_events_prepare, muzzle_prepare, swap_energy_prepare };
struct StormboltSessionResult {
    StormboltSessionState state;
    StormboltSessionIssue issue;
    MH_STATUS native_status = MH_OK;
    bool ok() const noexcept { return issue == StormboltSessionIssue::none; }
};

// Owns shared state and lifecycle/shell/coefficient/action components for process lifetime.
// The supplied transaction is ALREADY open and may contain queued quest hooks.
// This adopts its commit/rollback boundary, never calls MH_Initialize again.
// Caller keeps the transaction and this session alive and game/patch writers
// quiescent until prepare+activate or restoration fully succeeds. No game launch,
// executable authentication, resource readiness, action scheduling or quiescence
// is established by this integration component itself.
template<std::size_t Capacity, class Transaction = HookTransaction>
class StormboltShootingSession {
    StormboltInstanceStore<Capacity> instances_;
    StormboltLifecycleHooks<Capacity> lifecycle_{instances_};
    StormboltShellHook<Capacity> shells_{instances_};
    StormboltCoefficientHook<Capacity> coefficients_{instances_};
    StormboltNormalToSpecialHook<Capacity> normal_to_special_{instances_};
    std::optional<StormboltSpecialActionSession<Capacity>> special_;
    std::optional<StormboltGatlingStartSession<Capacity>> start_;
    std::optional<StormboltMiddleSession<Capacity>> middle_;
    std::optional<StormboltMiddleSession<Capacity,true>> primary_shoot_;
    std::optional<StormboltEffectProviderIdentity> last_identity_;
    std::optional<StormboltLastSession<Capacity>> last_;
    std::optional<StormboltWeaponUpdateHook<Capacity>> update_;
    std::optional<StormboltPowerCellSession<Capacity,Capacity>> power_cell_;
    std::optional<StormboltSourceTransitionLifecycle<Capacity>> source_transition_;
    std::optional<StormboltSourceTransitionConditionHook<Capacity>> source_condition_;
    StormboltSourceTransitionConditionBindings source_condition_bindings_{};
    StormboltSourceTransitionPredicateApi source_condition_api_{};
    std::optional<StormboltSourceManagerLifecycle<Capacity>> source_manager_;
    std::optional<StormboltConditionReadiness<Capacity>> source_readiness_;
    std::optional<StormboltEvadeReloadEffectSession<Capacity>> evade_;
    std::optional<StormboltEvadeEffectReadiness> evade_readiness_;
    std::optional<StormboltChargeHudHook> charge_hud_;
    std::optional<StormboltOperationHintHook<>> operation_hint_;
    std::optional<StormboltOperationHintHook<true>> training_hint_;
    std::optional<StormboltItemWindowHook> itemwindow_;
    std::optional<StormboltOrdinaryReticleHook> ordinary_reticle_;
    std::optional<StormboltMotionEventHook<Capacity>> motion_events_;
    std::optional<StormboltMuzzleEffectHook<Capacity>> muzzle_;
    std::optional<StormboltSwapEnergyHooks<Capacity>> swap_energy_;
    std::optional<StormboltItemWindowMode> itemwindow_mode_;
    Transaction* transaction_ = nullptr;
    StormboltSessionState state_ = StormboltSessionState::fresh;
    bool host_coordinated_=false, publications_restored_=false;

    static bool consistent(const StormboltNativeBindings& bindings) noexcept {
        const auto& life = bindings.lifecycle;
        const auto& shell = bindings.shell;
        const auto& coefficient = bindings.coefficients;
        const auto& action = bindings.normal_to_special;
        if (!life.allocate || !life.place || !life.destroy || !shell.create_entry ||
            !shell.record_at || !shell.caller_return || !shell.weapon_vtable || !shell.list_vtable ||
            !coefficient.original || !coefficient.caller_return || !coefficient.info_vtable ||
            !coefficient.parameter_vtable || shell.weapon_vtable != coefficient.weapon_vtable ||
            shell.weapon_vtable > (std::numeric_limits<std::uintptr_t>::max)() - 0x3D8 ||
            coefficient.slot != reinterpret_cast<void*>(shell.weapon_vtable + 0x3D8)) return false;
        if (!action.original || !action.actor_vtable || !action.action_vtable ||
            !action.event_object || !action.query || action.weapon_vtable != shell.weapon_vtable ||
            action.action_vtable > (std::numeric_limits<std::uintptr_t>::max)() - 0x38 ||
            action.slot != reinterpret_cast<void*>(action.action_vtable + 0x38) ||
            action.slot == coefficient.slot) return false;
        const void* entries[]{life.allocate, life.place, life.destroy, shell.create_entry,life.array_place};
        for (unsigned i = 0; i < 5; ++i)
            for (unsigned j = 0; j < i; ++j) if (entries[i] && entries[i] == entries[j]) return false;
        return true;
    }
    StormboltSessionResult failed(StormboltSessionIssue issue, MH_STATUS status = MH_OK) {
        // A shared host owns rollback across ALL modules. Retain our complete
        // graph until it restores every module, stops once, then releases.
        if(host_coordinated_) {
            state_=StormboltSessionState::recovery_required;
            return {state_,issue,status};
        }
        const auto recovery = restore();
        if (!recovery.ok()) return recovery;
        return {state_, issue, status};
    }
public:
    StormboltShootingSession() = default;
    StormboltShootingSession(const StormboltShootingSession&) = delete;
    StormboltShootingSession& operator=(const StormboltShootingSession&) = delete;
    StormboltInstanceStore<Capacity>& instances() noexcept { return instances_; }
    StormboltSessionResult attach_itemwindow_mode(void* site) {
        if(state_!=StormboltSessionState::prepared || itemwindow_mode_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        itemwindow_mode_.emplace(transaction_);
        if(!itemwindow_mode_->prepare(site))return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionState state() const noexcept { return state_; }
    // Select before prepare. Convenience activate/restore then cannot commit or
    // stop the host's transaction. The caller establishes native quiescence.
    bool use_shared_transaction_coordinator() noexcept {
        if(state_!=StormboltSessionState::fresh || transaction_) return false;
        host_coordinated_=true;return true;
    }
    auto* power_cell() noexcept {return power_cell_ ? &*power_cell_ : nullptr;}
    auto* source_transition() noexcept {return source_transition_ ? &*source_transition_ : nullptr;}
    auto* source_manager() noexcept {return source_manager_ ? &*source_manager_ : nullptr;}
    auto* source_readiness() noexcept {return source_readiness_ ? &*source_readiness_ : nullptr;}
    auto* evade_effect() noexcept {return evade_ ? &*evade_ : nullptr;}
    auto* evade_readiness() noexcept {return evade_readiness_ ? &*evade_readiness_ : nullptr;}
    StormboltSessionResult attach_swap_energy(const StormboltSwapEnergyBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || swap_energy_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        swap_energy_.emplace();
        const auto status=swap_energy_->prepare(*transaction_,bindings);
        if(status!=MH_OK) return failed(StormboltSessionIssue::swap_energy_prepare,status);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_motion_events(const StormboltMotionEventBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || motion_events_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        motion_events_.emplace(instances_);
        const auto status=motion_events_->prepare(*transaction_,bindings);
        if(status!=MH_OK) return failed(StormboltSessionIssue::motion_events_prepare,status);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_muzzle_effect(const StormboltMuzzleEffectBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || muzzle_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        muzzle_.emplace(instances_);
        const auto status=muzzle_->prepare(*transaction_,bindings);
        if(status!=MH_OK) return failed(StormboltSessionIssue::muzzle_prepare,status);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_charge_hud(const StormboltChargeHudBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || charge_hud_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        charge_hud_.emplace();
        if(!charge_hud_->install(bindings)) return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_operation_hint(const StormboltOperationHintBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || operation_hint_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        operation_hint_.emplace();
        if(!operation_hint_->install(bindings)) return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_training_hint(const StormboltOperationHintBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || training_hint_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        training_hint_.emplace();
        if(!training_hint_->install(bindings)) return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_ordinary_reticle(const StormboltOrdinaryReticleBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || ordinary_reticle_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        ordinary_reticle_.emplace();
        if(!ordinary_reticle_->install(bindings)) return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_itemwindow(const StormboltItemWindowBindings& bindings) {
        if(state_!=StormboltSessionState::prepared || itemwindow_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        itemwindow_.emplace();
        if(!itemwindow_->install(bindings)) return failed(StormboltSessionIssue::hud_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_native_evade_effect(const StormboltNativeBindings& bindings,
        const StormboltEvadeEffectReadinessBindings& readiness,
        bool (*dispatch_ready)(void*)=nullptr,void* dispatch_context=nullptr) {
        if(state_!=StormboltSessionState::prepared || evade_ || evade_readiness_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto expected=stormbolt_evade_effect_readiness_bindings(bindings);
        const auto& p=readiness.provider;const auto& e=expected.provider;
        if(!readiness.synchronous_borrow_verified || !readiness.resource_ready ||
            readiness.actor_vtable!=expected.actor_vtable || readiness.weapon_vtable!=expected.weapon_vtable ||
            readiness.epv_resource_vtable!=expected.epv_resource_vtable ||
            readiness.efx_resource_vtable!=expected.efx_resource_vtable ||
            !(readiness.mesh==expected.mesh) ||
            readiness.component_vtable!=expected.component_vtable || p.singleton_slot!=e.singleton_slot ||
            p.vtable!=e.vtable || p.dti!=e.dti || p.dti_vtable!=e.dti_vtable || p.dti_getter!=e.dti_getter ||
            p.record_create!=e.record_create || p.create!=e.create)
            return failed(StormboltSessionIssue::evade_prepare);
        evade_readiness_.emplace(readiness,dispatch_ready,dispatch_context);
        return attach_native_evade_effect(bindings,&StormboltEvadeEffectReadiness::callback,&*evade_readiness_);
    }
    StormboltSessionResult attach_native_evade_effect(const StormboltNativeBindings& bindings,
        bool (*ready)(void*,void*),void* context=nullptr) {
        auto api=bindings.evade_effect;api.ready=ready;api.context=context;
        return attach_evade_effect(bindings.evade_refill_call,api,bindings.evade_refill_expected);
    }
    StormboltSessionResult attach_evade_effect(std::uintptr_t site,
        StormboltEvadeReloadEffectBindings api,const std::array<unsigned char,5>& expected) {
        if(state_!=StormboltSessionState::prepared || evade_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        evade_.emplace();
        if(!evade_->prepare(instances_,site,api,expected,transaction_)) return failed(StormboltSessionIssue::evade_prepare);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_native_source_conditions(const StormboltNativeBindings& bindings,
        const StormboltConditionReadinessBindings& readiness,
        bool (*dispatch_ready)(void*)=nullptr,void* dispatch_context=nullptr) {
        if(state_!=StormboltSessionState::prepared || !source_transition_ || source_condition_ || source_manager_ || source_readiness_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto expected=stormbolt_condition_readiness_bindings(bindings);
        if(!readiness.synchronous_borrow_verified || readiness.actor_vtable!=expected.actor_vtable ||
            readiness.weapon_vtable!=expected.weapon_vtable || readiness.parameter_vtable!=expected.parameter_vtable ||
            readiness.manager_vtable!=expected.manager_vtable || readiness.authority_slot!=expected.authority_slot ||
            readiness.command_system_slot!=expected.command_system_slot || readiness.player_manager_slot!=expected.player_manager_slot)
            return failed(StormboltSessionIssue::source_condition_prepare);
        source_readiness_.emplace(instances_,readiness,dispatch_ready,dispatch_context);
        using Ready=StormboltConditionReadiness<Capacity>;
        auto result=attach_source_condition(bindings,stormbolt_source_transition_field_api(bindings),nullptr,
            &Ready::ready_setbomb,&*source_readiness_);
        if(!result.ok()) return result;
        return attach_source_manager(bindings,&Ready::ready_shoot,&*source_readiness_);
    }
    StormboltSessionResult attach_source_manager(const StormboltNativeBindings& bindings,
        bool (*ready)(void*,void*,void*),void* context=nullptr) {
        if(state_!=StormboltSessionState::prepared || !source_condition_ || source_manager_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto api=stormbolt_source_manager_api(bindings,ready,context);
        if(!ready || api.condition.actor_vtable!=bindings.normal_to_special.actor_vtable ||
            api.condition.weapon_vtable!=bindings.shell.weapon_vtable)
            return failed(StormboltSessionIssue::source_manager_prepare);
        source_manager_.emplace(instances_);
        const auto status=source_manager_->prepare(*transaction_,bindings.source_manager,api);
        if(status!=MH_OK) return failed(StormboltSessionIssue::source_manager_prepare,status);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_source_condition(const StormboltNativeBindings& bindings,
        const StormboltSourceTransitionPredicateApi& api,bool (*ready)(void*,void*),
        bool (*ready_with_context)(void*,void*,void*)=nullptr,void* ready_context=nullptr) {
        if(state_!=StormboltSessionState::prepared || !source_transition_ || source_condition_)
            return {state_,StormboltSessionIssue::wrong_state};
        auto condition=bindings.source_transition_condition;condition.ready=ready;
        condition.ready_with_context=ready_with_context;condition.ready_context=ready_context;
        if(!api.complete() || !condition.original || !condition.transition_vtable || (!ready && !ready_with_context) ||
            condition.actor_vtable!=bindings.normal_to_special.actor_vtable ||
            condition.weapon_vtable!=bindings.shell.weapon_vtable)
            return failed(StormboltSessionIssue::source_condition_prepare);
        source_condition_.emplace(source_transition_->instances());
        source_condition_bindings_=condition;source_condition_api_=api;
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_source_transition(const StormboltNativeBindings& bindings,
        decltype(StormboltSourceTransitionCreateApi::allocate) allocate) {
        if(state_!=StormboltSessionState::prepared || source_transition_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        source_transition_.emplace();
        const auto status=source_transition_->prepare(*transaction_,
            stormbolt_source_transition_create_api(bindings,allocate),bindings.source_transition);
        if(status!=MH_OK) return failed(StormboltSessionIssue::source_transition_prepare,status);
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult attach_power_cell(const StormboltNativeBindings& bindings,
        const std::array<unsigned char,7>& status_expected,
        const std::array<unsigned char,7>& timer_expected={0xE8,0x83,0xA9,0xD9,0,0x84,0xC0},
        const StormboltRemoteUpdateBindings& remote={},
        const StormboltSyncPackBindings& producer={},
        const std::array<std::uintptr_t,3>& growth_sites={},
        const StormboltPowerCellCompleteGrowthBindings& growth={}) {
        if (state_!=StormboltSessionState::prepared || power_cell_ || !transaction_->can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        if (!bindings.power_cell.construct || !bindings.power_cell.reset || !bindings.power_cell.destroy ||
            !bindings.power_cell.status_vtable || !bindings.loop_construct || !bindings.loop_destroy ||
            !bindings.loop_vtable || !bindings.power_cell_status_update_site || !bindings.loop_update_site ||
            !bindings.power_cell_status_manager_slot)
            return failed(StormboltSessionIssue::invalid_bindings);
        power_cell_.emplace(bindings,remote,producer,growth_sites,growth);
        if (!power_cell_->prepare(*transaction_,status_expected,timer_expected)) return failed(StormboltSessionIssue::power_cell_prepare);
        return {state_,StormboltSessionIssue::none};
    }

    StormboltSessionResult prepare_with_update(Transaction& transaction,const StormboltNativeBindings& bindings,
        std::uint32_t color,const std::array<float,4>& low,const std::array<float,4>& high) {
        if (state_!=StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto& hook=bindings.weapon_update;
        const auto& api=bindings.stage_update;
        const auto& r=api.routes;
        if (!hook.ready || !hook.original || !hook.weapon_vtable || (hook.weapon_vtable&7) ||
            hook.weapon_vtable!=bindings.shell.weapon_vtable ||
            hook.weapon_vtable>(std::numeric_limits<std::uintptr_t>::max)()-0x228 ||
            !api.gate.authority_slot || !api.gate.query || !api.descriptor || !api.ammo_items ||
            !r.input.query || !r.trigger.primary || !r.trigger.alternate_gate || !r.attachment ||
            !r.effect.active || !r.effect.request || !r.effect.assign ||
            !r.cleanup.active || !r.cleanup.stop || !r.cleanup.assign ||
            !r.owner.manager_slot || !r.owner.local_player || !r.actor_flag ||
            !r.color.active || !r.color.set_color || !r.material.write_vector)
            return {state_,StormboltSessionIssue::invalid_bindings};
        const auto slot=hook.weapon_vtable+0x228;
        if (slot==reinterpret_cast<std::uintptr_t>(bindings.normal_to_special.slot) ||
            slot==reinterpret_cast<std::uintptr_t>(bindings.coefficients.slot))
            return {state_,StormboltSessionIssue::invalid_bindings};
        for (const auto table : {bindings.last_hook.action_vtable,bindings.middle_hook.action_vtable,
            bindings.gatling_hook.action_vtable,bindings.special_to_normal.hook.action_vtable}) {
            if (table>(std::numeric_limits<std::uintptr_t>::max)()-0x48)
                return {state_,StormboltSessionIssue::invalid_bindings};
            for (std::uintptr_t offset : {0x30u,0x38u,0x40u})
                if (slot==table+offset) return {state_,StormboltSessionIssue::invalid_bindings};
        }
        const auto result=prepare_with_last(transaction,bindings);
        if (!result.ok()) return result;
        update_.emplace(instances_,api,color,low,high);
        if (!update_->install(hook)) return failed(StormboltSessionIssue::update_pointer);
        return {state_,StormboltSessionIssue::none};
    }

    StormboltSessionResult prepare_with_last(Transaction& transaction,const StormboltNativeBindings& bindings) {
        if (state_!=StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto& entry=bindings.last_entry;
        const auto& update=bindings.last_update;
        const auto& hook=bindings.last_hook;
        if (!entry.mode || !entry.parameter || !entry.request_motion || !update.event_object || !update.difference ||
            !update.origin || !update.fire || !update.actor_flag || !update.complete ||
            !hook.ready || !hook.original_enter || !hook.original_update || !hook.original_exit || !hook.action_vtable ||
            (hook.action_vtable&7) || hook.action_vtable>(std::numeric_limits<std::uintptr_t>::max)()-0x48 ||
            hook.actor_vtable!=bindings.normal_to_special.actor_vtable || hook.weapon_vtable!=bindings.shell.weapon_vtable)
            return {state_,StormboltSessionIssue::invalid_bindings};
        for (std::uintptr_t offset : {0x30u,0x38u,0x40u}) {
            const auto slot=hook.action_vtable+offset;
            if (slot==reinterpret_cast<std::uintptr_t>(bindings.coefficients.slot) ||
                slot==reinterpret_cast<std::uintptr_t>(bindings.normal_to_special.slot))
                return {state_,StormboltSessionIssue::invalid_bindings};
            for (const auto table : {bindings.middle_hook.action_vtable,bindings.gatling_hook.action_vtable,
                bindings.special_to_normal.hook.action_vtable}) {
                if (table>(std::numeric_limits<std::uintptr_t>::max)()-0x48)
                    return {state_,StormboltSessionIssue::invalid_bindings};
                for (std::uintptr_t other : {0x30u,0x38u,0x40u})
                    if (slot==table+other) return {state_,StormboltSessionIssue::invalid_bindings};
            }
        }
        const auto result=prepare_with_middle(transaction,bindings,bindings.middle_entry,bindings.middle_hook);
        if (!result.ok()) return result;
        last_identity_.emplace(bindings.special_to_normal.provider_identity);
        auto effects=bindings.special_to_normal.effects;
        effects.create=nullptr; effects.provider_resolver_context=&*last_identity_;
        effects.resolve_provider=resolve_stormbolt_effect_provider;
        last_.emplace(instances_,entry,update,effects,hook);
        if (!last_->install()) return failed(StormboltSessionIssue::last_pointer);
        return {state_,StormboltSessionIssue::none};
    }

    StormboltSessionResult prepare_with_middle(Transaction& transaction,const StormboltNativeBindings& bindings,
        const StormboltMiddleEntryApi& entry,const StormboltSpecialToNormalBindings& hook) {
        if (state_!=StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_,StormboltSessionIssue::wrong_state};
        const auto& prepare=bindings.middle_prepare;
        const auto& fire=bindings.middle_fire;
        if (!entry.mode || !entry.needs_parameter || !entry.parameter || !entry.read_stage || !entry.write_stage ||
            !prepare.animation.complete() || !prepare.difference || !prepare.rate || !fire.event_object || !fire.origin || !fire.fire ||
            !hook.ready || !hook.original_enter || !hook.original_update || !hook.action_vtable ||
            (hook.action_vtable&7) || hook.action_vtable>(std::numeric_limits<std::uintptr_t>::max)()-0x48 ||
            hook.actor_vtable!=bindings.normal_to_special.actor_vtable || hook.weapon_vtable!=bindings.shell.weapon_vtable)
            return {state_,StormboltSessionIssue::invalid_bindings};
        for (std::uintptr_t offset : {0x30u,0x38u}) {
            const auto slot=hook.action_vtable+offset;
            if (slot==reinterpret_cast<std::uintptr_t>(bindings.coefficients.slot) ||
                slot==reinterpret_cast<std::uintptr_t>(bindings.normal_to_special.slot))
                return {state_,StormboltSessionIssue::invalid_bindings};
            for (const auto table : {bindings.gatling_hook.action_vtable,bindings.special_to_normal.hook.action_vtable})
                for (std::uintptr_t other : {0x30u,0x38u,0x40u})
                    if (slot==table+other) return {state_,StormboltSessionIssue::invalid_bindings};
        }
        const auto prepared=prepare_with_start(transaction,bindings);
        if (!prepared.ok()) return prepared;
        middle_.emplace(instances_,entry,prepare,fire,prepare.animation,hook);
        if (!middle_->install()) return failed(StormboltSessionIssue::middle_pointer);
        if(bindings.primary_shoot_hook.action_vtable) {
            auto primary=bindings.primary_shoot_hook;
            if(!primary.ready || primary.action_vtable==hook.action_vtable ||
                primary.action_vtable==bindings.special_to_normal.hook.action_vtable ||
                primary.action_vtable==bindings.gatling_hook.action_vtable ||
                primary.actor_vtable!=hook.actor_vtable || primary.weapon_vtable!=hook.weapon_vtable)
                return failed(StormboltSessionIssue::invalid_bindings);
            primary_shoot_.emplace(instances_,entry,prepare,fire,prepare.animation,primary);
            if(!primary_shoot_->install()) return failed(StormboltSessionIssue::middle_pointer);
        }
        return {state_,StormboltSessionIssue::none};
    }

    StormboltSessionResult prepare_with_start(Transaction& transaction, const StormboltNativeBindings& bindings) {
        if (state_ != StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_, StormboltSessionIssue::wrong_state};
        const auto& api=bindings.gatling_entry;
        const auto& hook=bindings.gatling_hook;
        if (!api.animation.complete() || !api.player_manager_slot || !api.local_player || !api.ammo_items ||
            !hook.ready || !hook.original_enter || !hook.original_update || !hook.action_vtable ||
            (hook.action_vtable & 7) || hook.action_vtable > (std::numeric_limits<std::uintptr_t>::max)()-0x48 ||
            hook.actor_vtable != bindings.normal_to_special.actor_vtable || hook.weapon_vtable != bindings.shell.weapon_vtable)
            return {state_,StormboltSessionIssue::invalid_bindings};
        for (std::uintptr_t offset : {0x30u,0x38u}) {
            const auto slot=hook.action_vtable+offset;
            if (slot==reinterpret_cast<std::uintptr_t>(bindings.coefficients.slot) ||
                slot==reinterpret_cast<std::uintptr_t>(bindings.normal_to_special.slot))
                return {state_,StormboltSessionIssue::invalid_bindings};
            for (std::uintptr_t other : {0x30u,0x38u,0x40u})
                if (slot==bindings.special_to_normal.hook.action_vtable+other)
                    return {state_,StormboltSessionIssue::invalid_bindings};
        }
        const auto prepared=prepare_full(transaction,bindings);
        if (!prepared.ok()) return prepared;
        start_.emplace(instances_,api,hook);
        if (!start_->install()) return failed(StormboltSessionIssue::start_pointer);
        return {state_,StormboltSessionIssue::none};
    }

    // Full action integration is explicit; prepare() remains a component-only
    // path for existing fixture/bootstrap development, not a complete patch.
    StormboltSessionResult prepare_full(Transaction& transaction, const StormboltNativeBindings& bindings) {
        if (state_ != StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_, StormboltSessionIssue::wrong_state};
        const auto& endpoints = bindings.special_to_normal;
        if (!consistent(bindings) || !StormboltSpecialActionSession<Capacity>::complete(endpoints) ||
            endpoints.hook.weapon_vtable != bindings.shell.weapon_vtable ||
            endpoints.hook.actor_vtable != bindings.normal_to_special.actor_vtable ||
            endpoints.hook.action_vtable == bindings.normal_to_special.action_vtable ||
            (endpoints.hook.action_vtable & 7) ||
            endpoints.hook.action_vtable > (std::numeric_limits<std::uintptr_t>::max)() - 0x48)
            return {state_, StormboltSessionIssue::invalid_bindings};
        for (std::uintptr_t offset : {0x30u, 0x38u, 0x40u}) {
            const auto slot = reinterpret_cast<void*>(endpoints.hook.action_vtable + offset);
            if (slot == bindings.coefficients.slot || slot == bindings.normal_to_special.slot)
                return {state_, StormboltSessionIssue::invalid_bindings};
        }
        const auto result = prepare(transaction, bindings);
        if (!result.ok()) return result;
        special_.emplace(instances_, endpoints);
        if (!special_->install()) return failed(StormboltSessionIssue::special_action_pointer);
        return {state_, StormboltSessionIssue::none};
    }

    StormboltSessionResult prepare(Transaction& transaction, const StormboltNativeBindings& bindings) {
        if (state_ != StormboltSessionState::fresh || !transaction.can_add_hooks())
            return {state_, StormboltSessionIssue::wrong_state};
        // Invalid input is rejected without taking ownership of the caller's
        // pending transaction. Once adopted, the single-owner path rolls back
        // as a unit; a shared host retains failures for its coordinated rollback.
        if (!consistent(bindings)) return {state_, StormboltSessionIssue::invalid_bindings};
        if(host_coordinated_ && !transaction.retain_failed_adds_for_coordinator())
            return {state_,StormboltSessionIssue::wrong_state};
        transaction_ = &transaction;
        state_ = StormboltSessionState::prepared;
        auto status = lifecycle_.add_to_transaction(transaction, bindings.lifecycle);
        if (status != MH_OK) return failed(StormboltSessionIssue::lifecycle_hooks, status);
        status = shells_.add_to_transaction(transaction, bindings.shell);
        if (status != MH_OK) return failed(StormboltSessionIssue::shell_hook, status);
        if (!coefficients_.install(bindings.coefficients))
            return failed(StormboltSessionIssue::coefficient_pointer);
        if (!bindings.detonate_update_native_inherited && !normal_to_special_.install(bindings.normal_to_special))
            return failed(StormboltSessionIssue::action_pointer);
        return {state_, StormboltSessionIssue::none};
    }
    StormboltSessionResult activate() {
        if (host_coordinated_ || state_ != StormboltSessionState::prepared)
            return {state_, StormboltSessionIssue::wrong_state};
        // HookTransaction::commit() rolls code hooks back immediately on error.
        // Retain them here so this session can restore every virtual slot first.
        const auto status = transaction_->commit_pending_without_rollback();
        if (status != MH_OK) return failed(StormboltSessionIssue::activation, status);
        return activate_after_shared_commit_while_quiescent();
    }
    StormboltSessionResult activate_after_shared_commit_while_quiescent() {
        if(state_!=StormboltSessionState::prepared || publications_restored_ ||
            !transaction_ || !transaction_->has_library_state() || !transaction_->last.active)
            return {state_,StormboltSessionIssue::wrong_state};
        if (power_cell_ && !power_cell_->activate_after_shared_commit())
            return failed(StormboltSessionIssue::power_cell_activation);
        if(source_transition_ && !source_transition_->activate_factory())
            return failed(StormboltSessionIssue::source_transition_activation);
        if(source_condition_ && !source_condition_->install(source_condition_bindings_,source_condition_api_))
            return failed(StormboltSessionIssue::source_condition_activation);
        if(evade_ && !evade_->activate()) return failed(StormboltSessionIssue::evade_activation);
        if(itemwindow_mode_ && !itemwindow_mode_->activate()) return failed(StormboltSessionIssue::hud_prepare);
        state_ = StormboltSessionState::enabled;
        return {state_, StormboltSessionIssue::none};
    }
    StormboltSessionResult restore() {
        if(state_==StormboltSessionState::fresh || state_==StormboltSessionState::stopped)
            return {state_,StormboltSessionIssue::none};
        if(host_coordinated_) return {state_,StormboltSessionIssue::wrong_state};
        const auto restored=restore_before_shared_stop_while_quiescent();
        if(!restored.ok() || !transaction_ || state_==StormboltSessionState::stopped) return restored;
        const auto status=transaction_->stop();
        if(status!=MH_OK || transaction_->has_library_state()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hook_restore,status};
        }
        return release_after_shared_stop_while_quiescent();
    }
    // No commit/stop here: the host restores every module before stopping once.
    // Callback waiting happens only after native producers have been stopped;
    // do not call this while suspended threads hold callback leases.
    StormboltSessionResult restore_before_shared_stop_while_quiescent() {
        if (state_ == StormboltSessionState::fresh || state_ == StormboltSessionState::stopped)
            return {state_, StormboltSessionIssue::none};
        if(publications_restored_) return {state_,StormboltSessionIssue::none};
        // Reverse installation order. Retain the complete dependency graph until
        // every slot/protection and code hook is restored, including retry paths.
        if(motion_events_) motion_events_->withdraw_while_quiescent();
        if(muzzle_) muzzle_->withdraw_while_quiescent();
        if(swap_energy_) swap_energy_->withdraw_while_quiescent();
        if(itemwindow_mode_ && !itemwindow_mode_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(ordinary_reticle_ && !ordinary_reticle_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(itemwindow_ && !itemwindow_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(training_hint_ && !training_hint_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(operation_hint_ && !operation_hint_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(charge_hud_ && !charge_hud_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        if(evade_ && !evade_->restore_patch()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::evade_restore};
        }
        if(source_manager_ && !source_manager_->restore_all()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::source_manager_restore};
        }
        if(source_condition_ && !source_condition_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::source_condition_restore};
        }
        if(source_transition_ && !source_transition_->restore_factory()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::source_transition_restore};
        }
        if (power_cell_ && !power_cell_->restore_update_patches()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::power_cell_restore};
        }
        if (update_ && !update_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::update_restore};
        }
        if (last_ && !last_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::last_restore};
        }
        if(primary_shoot_ && !primary_shoot_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::middle_restore};
        }
        if (middle_ && !middle_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::middle_restore};
        }
        if (start_ && !start_->restore()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::start_restore};
        }
        if (special_ && !special_->restore()) {
            state_ = StormboltSessionState::recovery_required;
            return {state_, StormboltSessionIssue::special_action_restore};
        }
        // Keep lifecycle tracking if either callback
        // is owned or protection restoration is pending. Never leave it referring
        // to a registry whose destruction tracking has already been removed.
        if (!normal_to_special_.restore()) {
            state_ = StormboltSessionState::recovery_required;
            return {state_, StormboltSessionIssue::action_pointer_restore};
        }
        if (!coefficients_.restore()) {
            state_ = StormboltSessionState::recovery_required;
            return {state_, StormboltSessionIssue::pointer_restore};
        }
        if(power_cell_ && !power_cell_->withdraw_updates_after_quiescence()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::power_cell_restore};
        }
        publications_restored_=true;
        state_=StormboltSessionState::recovery_required;
        return {state_,StormboltSessionIssue::none};
    }
    StormboltSessionResult release_after_shared_stop_while_quiescent() {
        if(state_==StormboltSessionState::fresh || state_==StormboltSessionState::stopped)
            return {state_,StormboltSessionIssue::none};
        if(!publications_restored_ || !transaction_ || transaction_->has_library_state())
            return {state_,StormboltSessionIssue::wrong_state};
        if(evade_ && !evade_->release_after_quiescence()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::evade_restore};
        }
        if (power_cell_ && !power_cell_->release_after_shared_stop()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::power_cell_restore};
        }
        if(source_transition_ && !source_transition_->release_after_stop()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::source_transition_restore};
        }
        if(source_manager_ && !source_manager_->release_after_stop()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::source_manager_restore};
        }
        if(itemwindow_mode_ && !itemwindow_mode_->release_after_shared_stop()) {
            state_=StormboltSessionState::recovery_required;
            return {state_,StormboltSessionIssue::hud_restore};
        }
        itemwindow_mode_.reset();
        state_ = StormboltSessionState::stopped;
        if(motion_events_) motion_events_->release_after_stop();
        motion_events_.reset();
        if(swap_energy_) swap_energy_->release_after_stop();
        swap_energy_.reset();
        if(muzzle_) muzzle_->release_after_stop();
        muzzle_.reset();
        charge_hud_.reset();
        operation_hint_.reset();
        training_hint_.reset();
        itemwindow_.reset();
        ordinary_reticle_.reset();
        evade_.reset();
        evade_readiness_.reset();
        source_manager_.reset();
        source_condition_.reset();
        source_readiness_.reset();
        source_transition_.reset();
        power_cell_.reset();
        update_.reset();
        last_.reset();
        last_identity_.reset();
        special_.reset();
        start_.reset();
        primary_shoot_.reset();middle_.reset();
        return {state_, StormboltSessionIssue::none};
    }
};
} // namespace stormslinger
