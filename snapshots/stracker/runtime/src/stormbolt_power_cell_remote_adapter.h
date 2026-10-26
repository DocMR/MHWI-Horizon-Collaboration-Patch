#pragma once
#include "stormbolt_power_cell_remote_commit.h"
#include "stormbolt_power_cell_remote_effects.h"

namespace stormslinger {
struct StormboltPowerCellRemoteInputs {
    StormboltPowerCellToken status_token{};
    StormboltPowerCellRemoteFlags flags{};
    bool previous_effect=false, owner_enabled=false;
    std::uint32_t threshold=0;
    void* associated=nullptr;
    bool associated_player=false, associated_local=false;
};
template<std::size_t StatusCapacity,std::size_t LoopCapacity>
bool process_stormbolt_power_cell_remote(
    const StormboltPowerCellRemoteInputs& inputs,
    StormboltPowerCellStore<StatusCapacity>& statuses,
    StormboltPowerCellLoopStore<LoopCapacity>& loops,
    const StormboltLoopAudioUpdateBindings& audio,
    StormboltEquipmentEffectRequest equipment,
    StormboltPowerCellRemoteDecision& decision,
    StormboltPowerCellPostEffectsResult& effects) {
    if (!commit_stormbolt_power_cell_remote(inputs.status_token,statuses,
        inputs.flags,inputs.previous_effect,inputs.owner_enabled,inputs.threshold,
        inputs.associated!=nullptr,inputs.associated_player,inputs.associated_local,
        decision)) return false;
    // A later native request failure cannot undo an already applied count.
    return apply_stormbolt_power_cell_remote_effects(decision,inputs.associated,
        loops,audio,equipment,effects);
}
} // namespace stormslinger
