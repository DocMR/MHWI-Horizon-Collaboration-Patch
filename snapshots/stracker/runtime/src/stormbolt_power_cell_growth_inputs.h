#pragma once
#include "stormbolt_power_cell_growth_gate.h"
#include "stormbolt_damage_exception_store.h"
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
struct StormboltPowerCellGrowthInputs {
    StormboltPowerCellGrowthGateInputs gate{};
    StormboltPowerCellToken status_token{};
    void* associated_actor=nullptr;
    float duration=0;
    void* first_actor=nullptr;
};
template<std::size_t S>
bool resolve_stormbolt_power_cell_growth_local_identity(StormboltPowerCellGrowthInputs& inputs,
    StormboltPowerCellStore<S>& statuses,void** managerSlot,void* (*localPlayer)(void*)) {
    std::uintptr_t manager=0;
    if (!managerSlot || !localPlayer || !read_stormbolt_field(
        reinterpret_cast<std::uintptr_t>(managerSlot),0,manager) || !manager) return false;
    void* local=localPlayer(reinterpret_cast<void*>(manager));
    StormboltPowerCellState state{};
    if (!statuses.snapshot(inputs.status_token,state)) return false;
    inputs.gate.associated_actor_is_local=local && local==inputs.associated_actor;
    return true;
}
// Collects the shared native/sidecar inputs. Local identity is deliberately
// left false until the separately authenticated player-manager query runs.
template<std::size_t S,std::size_t D>
bool read_stormbolt_power_cell_growth_inputs(void* component,const void* event,const void* result,
    std::uintptr_t damageVtable,StormboltDamageExceptionStore<D>& damageStore,
    StormboltPowerCellStore<S>& statuses,const StormboltLoopInputBindings& bindings,
    StormboltSkillValueQuery query,StormboltPowerCellGrowthInputs& output) {
    const auto address=reinterpret_cast<std::uintptr_t>(component);
    StormboltDamageExceptionToken damageToken{};
    StormboltDamageExceptionState damage{};
    std::uintptr_t vtable=0,owner=0,status=0,associated=0,resource=0;
    StormboltPowerCellGrowthInputs candidate{};
    if (!component || !event || !result || !damageVtable ||
        !damageStore.token_for_live_address(address,damageToken) || !damageStore.snapshot(damageToken,damage) ||
        !read_stormbolt_field(address,0,vtable) || vtable!=damageVtable ||
        !read_stormbolt_field(address,0x60,owner) || !owner ||
        !read_stormbolt_field(owner,0x7D20,status) || !status ||
        !read_stormbolt_field(status,0,vtable) || vtable!=bindings.status_vtable ||
        !statuses.token_for_live_address(status,candidate.status_token)) return false;
    StormboltPowerCellState state{};
    StormboltPowerCellLoopUpdateInputs first{},second{};
    std::uint8_t enabled=0,associatedFlags=0;
    if (!statuses.snapshot(candidate.status_token,state) ||
        !read_stormbolt_loop_skill_gate(reinterpret_cast<void*>(owner),bindings.skill_vtable,query,first) ||
        !read_stormbolt_field(status,8,associated) ||
        !read_stormbolt_field(owner,0x8938,enabled) ||
        !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(result),0,candidate.gate.amount) ||
        !read_stormbolt_power_cell_event_metadata(event,candidate.gate.metadata) ||
        !bindings.resource_slot || !read_stormbolt_field(bindings.resource_slot,0,resource) || !resource ||
        !read_stormbolt_field(resource,0,vtable) || vtable!=bindings.resource_vtable ||
        !read_stormbolt_field(resource,0xA8C,candidate.gate.threshold) ||
        !read_stormbolt_field(resource,0xA94,candidate.duration)) return false;
    if (associated && (!read_stormbolt_field(associated,0x60,associatedFlags) ||
        !read_stormbolt_loop_skill_gate(reinterpret_cast<void*>(associated),bindings.skill_vtable,query,second))) return false;
    candidate.gate.first_actor_powered=first.owner_flag && first.resource_index_present && first.resource_count>0;
    candidate.gate.associated_actor_powered=second.owner_flag && second.resource_index_present && second.resource_count>0;
    candidate.gate.absorbed_positive_input=damage.absorbed_positive_input;
    candidate.gate.event_eligible=state.event_eligible;
    candidate.gate.owner_enabled=(enabled&1)!=0;
    candidate.gate.associated_actor_is_player=associated && (associatedFlags&1)!=0;
    candidate.gate.count=state.count;
    candidate.associated_actor=reinterpret_cast<void*>(associated);
    candidate.first_actor=reinterpret_cast<void*>(owner);
    output=candidate;return true;
}
} // namespace stormslinger
