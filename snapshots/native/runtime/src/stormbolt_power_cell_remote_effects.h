#pragma once
#include "stormbolt_power_cell_remote_decision.h"
#include "stormbolt_power_cell_growth_effects.h"
#include "stormbolt_power_cell_post_effects.h"

namespace stormslinger {
// PS remote application: no local prompt; combine effects precede both body
// requests. Count classification has already committed independently.
template<std::size_t Capacity>
bool apply_stormbolt_power_cell_remote_effects(
    StormboltPowerCellRemoteDecision decision, void* associated,
    StormboltPowerCellLoopStore<Capacity>& loops,
    const StormboltLoopAudioUpdateBindings& audio,
    StormboltEquipmentEffectRequest equipment,
    StormboltPowerCellPostEffectsResult& output) {
    output={};
    if (!decision.continue_effects) return true;
    const auto result=decision.reached_effect
        ? StormboltPowerCellGrowthResult::reached_threshold
        : StormboltPowerCellGrowthResult::below_threshold;
    if (!apply_stormbolt_power_cell_growth_effects(result,associated,loops,audio,nullptr,false))
        return false;
    return request_stormbolt_power_cell_post_effects(associated,equipment,output);
}
} // namespace stormslinger
