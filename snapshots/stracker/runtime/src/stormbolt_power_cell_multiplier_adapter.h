#pragma once
#include "stormbolt_power_cell_store.h"
#include "stormbolt_power_cell_parameters.h"
#include "stormbolt_power_cell_multiplier.h"

namespace stormslinger {
enum class StormboltPowerCellMultiplierResult { ineligible, unavailable, applied };
// Caller authenticates associated actor/skill eligibility, resource identity,
// borrowed lifetimes and serialized state access for this calculation. Call at
// the verified intermediate arithmetic boundary, before native skill14 math.
// This is not a hook, and it does not infer eligibility from weapon identity.
template<std::size_t Capacity>
StormboltPowerCellMultiplierResult apply_stormbolt_power_cell_multiplier(
    StormboltPowerCellStore<Capacity>& store, StormboltPowerCellToken token,
    bool skill_eligible, const void* resource, float& accumulator) {
    if (!skill_eligible) return StormboltPowerCellMultiplierResult::ineligible;
    StormboltPowerCellState state{};
    if (!store.snapshot(token,state)) return StormboltPowerCellMultiplierResult::unavailable;
    StormboltPowerCellParameters parameters{};
    if (!read_stormbolt_power_cell_parameters(resource,parameters))
        return StormboltPowerCellMultiplierResult::unavailable;
    const auto index=select_stormbolt_power_cell_multiplier(
        state.count,parameters.threshold,parameters.related_count);
    accumulator*=parameters.multipliers[index];
    return StormboltPowerCellMultiplierResult::applied;
}
} // namespace stormslinger
