#pragma once
#include "stormbolt_power_cell_store.h"
#include "stormbolt_power_cell_parameters.h"

namespace stormslinger {
enum class StormboltPowerCellGrowthResult { unchanged, below_threshold, reached_threshold };
// PS2CEE3D1/3DE +2CEE494..4CD. Invoke only after all native event/owner
// gates pass. This is not a per-shot trigger or a replacement for those gates.
// Below threshold the caller marks the combine-component growth latch;
// reaching threshold instead requires prompt then loop-start native ordering.
inline StormboltPowerCellGrowthResult advance_stormbolt_power_cell_accepted_event(
    StormboltPowerCellState& state,const StormboltPowerCellParameters& parameters) noexcept {
    if (state.count>=parameters.threshold) return StormboltPowerCellGrowthResult::unchanged;
    ++state.count;
    if (state.count<parameters.threshold) return StormboltPowerCellGrowthResult::below_threshold;
    state.timer=parameters.duration;
    return StormboltPowerCellGrowthResult::reached_threshold;
}
} // namespace stormslinger
