#pragma once
#include "stormbolt_power_cell_store.h"
#include "stormbolt_power_cell_parameters.h"

namespace stormslinger {
enum class StormboltPowerCellPromotion { not_required, unavailable, promoted };
// PS4998ADB..4998AFB. Caller has selected the complete special self-damage
// branch and authenticated current parameters, actor and synchronous lifetimes.
// A promoted result requires the caller to run the original ordered local
// prompt/audio/body-effect sequence before dispatching health adjustment.
template<std::size_t Capacity>
StormboltPowerCellPromotion promote_stormbolt_power_cell_for_self_damage(
    StormboltPowerCellStore<Capacity>& store, StormboltPowerCellToken token,
    bool special_branch, std::uint32_t shot_stage,
    const StormboltPowerCellParameters& parameters) {
    if (!special_branch || shot_stage<8) return StormboltPowerCellPromotion::not_required;
    StormboltPowerCellState snapshot{};
    if (!store.snapshot(token,snapshot)) return StormboltPowerCellPromotion::unavailable;
    if (snapshot.count>=parameters.threshold) return StormboltPowerCellPromotion::not_required;
    if (!store.transact(token,[&](auto& state) noexcept {
        if (state.count!=snapshot.count) return false;
        state.count=parameters.threshold;
        state.timer=parameters.duration;
        return true;
    })) return StormboltPowerCellPromotion::unavailable;
    return StormboltPowerCellPromotion::promoted;
}
} // namespace stormslinger
