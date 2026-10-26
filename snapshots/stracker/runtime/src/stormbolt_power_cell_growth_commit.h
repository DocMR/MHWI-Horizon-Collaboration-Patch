#pragma once
#include "stormbolt_power_cell_growth_inputs.h"
#include "stormbolt_power_cell_event_growth.h"

namespace stormslinger {
template<std::size_t Capacity>
bool commit_stormbolt_power_cell_growth(const StormboltPowerCellGrowthInputs& inputs,
    StormboltPowerCellStore<Capacity>& store,StormboltPowerCellGrowthResult& output) {
    auto result=StormboltPowerCellGrowthResult::unchanged;
    const bool committed=store.transact(inputs.status_token,[&](auto& state) noexcept {
        auto gate=inputs.gate;
        gate.count=state.count;
        gate.event_eligible=state.event_eligible;
        if (!stormbolt_power_cell_growth_gate(gate)) return true;
        StormboltPowerCellParameters parameters{};
        parameters.threshold=gate.threshold;parameters.duration=inputs.duration;
        result=advance_stormbolt_power_cell_accepted_event(state,parameters);
        return true;
    });
    if (!committed) return false;
    output=result;return true;
}
} // namespace stormslinger
