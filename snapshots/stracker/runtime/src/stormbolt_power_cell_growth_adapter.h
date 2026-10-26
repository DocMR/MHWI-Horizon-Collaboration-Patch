#pragma once
#include "stormbolt_power_cell_growth_effects.h"

namespace stormslinger {
struct StormboltPowerCellGrowthBindings {
    std::uintptr_t damage_vtable=0;
    StormboltLoopAudioUpdateBindings loop{};
    StormboltSkillPrompt prompt=nullptr;
    void** manager_slot=nullptr;
    void* (*local_player)(void*)=nullptr;
};
template<std::size_t S,std::size_t D,std::size_t L>
bool process_stormbolt_power_cell_growth_event(void* component,const void* event,const void* result,
    StormboltDamageExceptionStore<D>& damages,StormboltPowerCellStore<S>& statuses,
    StormboltPowerCellLoopStore<L>& loops,const StormboltPowerCellGrowthBindings& bindings,
    StormboltPowerCellGrowthResult& output) {
    StormboltPowerCellGrowthInputs inputs{};
    if (!read_stormbolt_power_cell_growth_inputs(component,event,result,bindings.damage_vtable,
        damages,statuses,bindings.loop.inputs,bindings.loop.query,inputs)) return false;
    auto preliminary=inputs.gate;
    preliminary.associated_actor_is_local=true;
    if (!stormbolt_power_cell_growth_gate(preliminary)) {
        output=StormboltPowerCellGrowthResult::unchanged;return true;
    }
    if (!resolve_stormbolt_power_cell_growth_local_identity(inputs,statuses,
        bindings.manager_slot,bindings.local_player)) return false;
    if (!commit_stormbolt_power_cell_growth(inputs,statuses,output)) return false;
    // output describes the committed state even if a later native effect fails.
    return apply_stormbolt_power_cell_growth_effects(output,inputs.associated_actor,
        loops,bindings.loop,bindings.prompt);
}
} // namespace stormslinger
