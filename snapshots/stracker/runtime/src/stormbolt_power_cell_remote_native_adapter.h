#pragma once
#include "stormbolt_power_cell_remote_inputs.h"

namespace stormslinger {
template<std::size_t StatusCapacity,std::size_t LoopCapacity>
bool process_stormbolt_power_cell_remote_native(
    const StormboltPowerCellRemoteCache& cache,StormboltPowerCellRemoteFlags flags,
    bool previous_effect,StormboltPowerCellStore<StatusCapacity>& statuses,
    StormboltPowerCellLoopStore<LoopCapacity>& loops,
    const StormboltLoopInputBindings& native_inputs,
    const StormboltLoopAudioUpdateBindings& audio,
    void** manager_slot,void* (*local_player)(void*),
    StormboltEquipmentEffectRequest equipment,
    StormboltPowerCellRemoteDecision& decision,
    StormboltPowerCellPostEffectsResult& effects) {
    StormboltPowerCellRemoteInputs inputs{};
    if (!read_stormbolt_power_cell_remote_inputs(cache,flags,previous_effect,
        statuses,native_inputs,inputs)) return false;
    // PS applies the count before querying remote effect identity. Suppress
    // effects during this first phase without changing count classification.
    if (!commit_stormbolt_power_cell_remote(inputs.status_token,statuses,flags,
        previous_effect,inputs.owner_enabled,inputs.threshold,inputs.associated!=nullptr,
        inputs.associated_player,true,decision)) return false;
    effects={};
    if (flags.effect && !previous_effect && inputs.associated &&
        !resolve_stormbolt_power_cell_remote_identity(inputs,statuses,
            manager_slot,local_player)) return false;
    decision=decide_stormbolt_power_cell_remote(flags,previous_effect,true,
        decision.count,inputs.threshold,inputs.associated!=nullptr,
        inputs.associated_player,inputs.associated_local);
    return apply_stormbolt_power_cell_remote_effects(decision,inputs.associated,
        loops,audio,equipment,effects);
}
} // namespace stormslinger
