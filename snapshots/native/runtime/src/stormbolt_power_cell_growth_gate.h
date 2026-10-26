#pragma once
#include "stormbolt_power_cell_event_context.h"

namespace stormslinger {
struct StormboltPowerCellGrowthGateInputs {
    bool first_actor_powered=false;
    float amount=0;
    bool absorbed_positive_input=false;
    bool associated_actor_powered=false;
    bool event_eligible=false;
    StormboltPowerCellEventMetadata metadata{};
    bool owner_enabled=false;
    std::uint32_t count=0,threshold=0;
    bool associated_actor_is_player=false;
    bool associated_actor_is_local=false;
};
// PS2CEE2B2..2CEE48E. Inputs must be gathered at the authenticated event
// boundary; this predicate alone does not establish native pointer lifetimes.
struct StormboltPowerCellGrowthDecision {bool continue_effects=false;bool increment=false;};
inline StormboltPowerCellGrowthDecision decide_stormbolt_power_cell_growth(
    const StormboltPowerCellGrowthGateInputs& in) noexcept {
    if (!in.first_actor_powered || !(in.amount>0 || in.absorbed_positive_input)) return {};
    // 2CEE46A clears CL then jumps back to metadata comparison. It bypasses
    // only the A14 eligibility requirement, never metadata exclusion.
    if (!(in.associated_actor_powered && in.count!=in.threshold) && !in.event_eligible) return {};
    if (!stormbolt_power_cell_metadata_allows_growth(in.metadata) || !in.owner_enabled) return {};
    return {true,in.count<in.threshold && in.associated_actor_is_player && in.associated_actor_is_local};
}
inline bool stormbolt_power_cell_growth_gate(const StormboltPowerCellGrowthGateInputs& in) noexcept {
    return decide_stormbolt_power_cell_growth(in).increment;
}
} // namespace stormslinger
