#pragma once
#include "stormbolt_power_cell_growth_adapter.h"
#include "stormbolt_power_cell_growth_sync.h"
#include "stormbolt_power_cell_post_effects.h"

namespace stormslinger {
struct StormboltPowerCellCompleteGrowthBindings {
    StormboltPowerCellGrowthBindings growth{};
    std::uintptr_t master_vtable=0;
    StormboltEquipmentEffectRequest equipment=nullptr;
};
template<std::size_t S,std::size_t D,std::size_t L,std::size_t N>
bool process_stormbolt_power_cell_complete_growth_event(
    void* component,const void* event,const void* result,
    StormboltDamageExceptionStore<D>& damages,StormboltPowerCellStore<S>& statuses,
    StormboltPowerCellLoopStore<L>& loops,StormboltPowerCellSyncStore<N>& sync,
    const StormboltPowerCellCompleteGrowthBindings& bindings,
    StormboltPowerCellGrowthResult& output,StormboltPowerCellPostEffectsResult& effects) {
    StormboltPowerCellGrowthInputs inputs{};
    const auto& growth=bindings.growth;
    if (!read_stormbolt_power_cell_growth_inputs(component,event,result,growth.damage_vtable,
        damages,statuses,growth.loop.inputs,growth.loop.query,inputs)) return false;
    auto preliminary=inputs.gate;preliminary.associated_actor_is_local=true;
    const auto decision=decide_stormbolt_power_cell_growth(preliminary);
    output=StormboltPowerCellGrowthResult::unchanged;effects={};
    if (!decision.continue_effects) return true;
    if (decision.increment) {
        if (!resolve_stormbolt_power_cell_growth_local_identity(inputs,statuses,
            growth.manager_slot,growth.local_player) ||
            !commit_stormbolt_power_cell_growth(inputs,statuses,output)) return false;
        if (!apply_stormbolt_power_cell_growth_effects(output,inputs.associated_actor,
            loops,growth.loop,growth.prompt) ||
            !mark_stormbolt_power_cell_sync_after_growth(output,inputs.associated_actor,
                growth.manager_slot,growth.local_player,bindings.master_vtable,sync)) return false;
    }
    // PS performs these on the first actor even when no increment occurred.
    return request_stormbolt_power_cell_post_effects(inputs.first_actor,bindings.equipment,effects);
}
} // namespace stormslinger
