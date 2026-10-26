#pragma once
#include "stormbolt_native_bindings.h"

namespace stormslinger {
// Explicit host integration, outside DllMain. The host keeps callbacks/contexts
// alive and native execution quiescent through start/stop, and establishes the
// synchronous native callback borrow. Null action/update callbacks select the
// owned typed checks; a null dependency callback selects the complete declared
// weapon-EPV root check. The native borrow remains mandatory.
struct StormboltRuntimeReadiness {
    unsigned version=1;
    decltype(StormboltSpecialToNormalBindings::ready) special=nullptr,gatling=nullptr,middle=nullptr,last=nullptr;
    decltype(StormboltWeaponUpdateBindings::ready) update=nullptr;
    // Optional expected native address; null uses the authenticated default heap.
    decltype(StormboltSourceTransitionCreateApi::allocate) allocate=nullptr;
    StormboltConditionReadinessBindings conditions{};
    StormboltEvadeEffectReadinessBindings evade{};
    // Original PS initializer:499907C color;499915B/4999191 HLSA globals.
    // counter<6 selects6F495A0 (57F5D20), otherwise6F49590 (57F5D30).
    // These are initial values, not a claim that runtime overrides are absent.
    std::uint32_t color=ps_stormbolt_color_default;
    std::array<float,4> low{3.473f,0.5f,1.0f,1.0f};
    std::array<float,4> high{4.6f,0.6f,1.0f,1.0f};
};
// Development-candidate policy selected by the user on 2026-10-16:
// rely on native synchronous callbacks to keep their object graph alive until
// return. These legacy flags authorize that contract; they are NOT a claim of
// static scheduler proof. Default/external readiness remains strict and false.
inline StormboltRuntimeReadiness stormbolt_native_callback_candidate_readiness() {
    StormboltRuntimeReadiness result;
    result.conditions.synchronous_borrow_verified=true;
    result.evade.synchronous_borrow_verified=true;
    return result;
}
// Actual equipment-gate consumer and offline fixture share this startup order.
// Never start shooting after a quest failure, and never hide shooting failure.
template<class Quest,class Shooting>
int stormbolt_start_native_callback_candidate(Quest quest,Shooting shooting) {
    const int result=quest();
    if(result) return result;
    return shooting(stormbolt_native_callback_candidate_readiness());
}
// Called by the production bridge AFTER executable/live-code binding. Address
// defaults belong to that authenticated profile, not to the external host.
// Explicit nonzero expectations remain intact for the existing identity checks.
// This never asserts native object borrow or resource readiness.
inline StormboltRuntimeReadiness stormbolt_configure_runtime_readiness(
    const StormboltNativeBindings& native,const StormboltRuntimeReadiness& request) {
    auto result=request;
    const auto conditions=stormbolt_condition_readiness_bindings(native);
    const auto evade=stormbolt_evade_effect_readiness_bindings(native);
    const auto fill=[](auto& value,const auto& expected) {if(!value) value=expected;};
    fill(result.conditions.actor_vtable,conditions.actor_vtable);
    fill(result.conditions.weapon_vtable,conditions.weapon_vtable);
    fill(result.conditions.parameter_vtable,conditions.parameter_vtable);
    fill(result.conditions.manager_vtable,conditions.manager_vtable);
    fill(result.conditions.authority_slot,conditions.authority_slot);
    fill(result.conditions.command_system_slot,conditions.command_system_slot);
    fill(result.conditions.player_manager_slot,conditions.player_manager_slot);
    fill(result.evade.weapon_vtable,evade.weapon_vtable);
    fill(result.evade.actor_vtable,evade.actor_vtable);
    fill(result.evade.component_vtable,evade.component_vtable);
    fill(result.evade.epv_resource_vtable,evade.epv_resource_vtable);
    fill(result.evade.efx_resource_vtable,evade.efx_resource_vtable);
    fill(result.evade.provider.singleton_slot,evade.provider.singleton_slot);
    fill(result.evade.provider.vtable,evade.provider.vtable);
    fill(result.evade.provider.dti,evade.provider.dti);
    fill(result.evade.provider.dti_vtable,evade.provider.dti_vtable);
    fill(result.evade.provider.dti_getter,evade.provider.dti_getter);
    fill(result.evade.provider.record_create,evade.provider.record_create);
    fill(result.evade.provider.create,evade.provider.create);
    fill(result.evade.mesh.container,evade.mesh.container);
    fill(result.evade.mesh.emitter,evade.mesh.emitter);
    fill(result.evade.mesh.parameter,evade.mesh.parameter);
    fill(result.evade.mesh.model,evade.mesh.model);
    fill(result.evade.mesh.positions,evade.mesh.positions);
    fill(result.evade.mesh.material,evade.mesh.material);
    fill(result.evade.mesh.material_manager_slot,evade.mesh.material_manager_slot);
    fill(result.evade.mesh.texture,evade.mesh.texture);
    fill(result.evade.mesh.group,evade.mesh.group);
    fill(result.evade.mesh.play_efx,evade.mesh.play_efx);
    if(!result.allocate)
        result.allocate=reinterpret_cast<decltype(result.allocate)>(native.default_heap.allocate);
    return result;
}
}
// Requires a started quest bridge and authenticated current-game image.
// 0: activated; positive: MinHook status; negative: input/code/session failure.
// Session activation is once per process; a stopped session is not restarted.
extern "C" __declspec(dllexport) int StormslingerStartShootingBridge(void* image,std::size_t image_bytes,
    const stormslinger::StormboltRuntimeReadiness* readiness);
extern "C" __declspec(dllexport) int StormslingerStopShootingBridge();
