#pragma once
#include <cstdint>

namespace stormslinger {
enum class StormboltPowerCellTimerResult { inactive, advanced, expired };
// PS2C55AE4..2C55B28. Inputs already come from an authenticated state component
// and native update step. Native ordered comparisons skip NaN at entry/expiry.
// Caller dispatches the skill-conditioned prompt removal after expiry; this
// helper does not claim native component offsets, ownership, or time units.
inline StormboltPowerCellTimerResult advance_stormbolt_power_cell_timer(
    std::uint32_t& count, float& timer, bool update_enabled, float step) noexcept {
    if (!update_enabled || !(timer>=0.0f)) return StormboltPowerCellTimerResult::inactive;
    timer-=step;
    if (!(timer<=0.0f)) return StormboltPowerCellTimerResult::advanced;
    count=0;
    timer=-1.0f;
    return StormboltPowerCellTimerResult::expired;
}
} // namespace stormslinger
