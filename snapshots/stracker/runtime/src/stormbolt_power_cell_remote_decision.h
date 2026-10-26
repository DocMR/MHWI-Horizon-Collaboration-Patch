#pragma once
#include <cstdint>

namespace stormslinger {
// Logical PS fields, not PC wire bit assignments. Transport mapping must
// preserve the existing PC state flags before supplying these values.
struct StormboltPowerCellRemoteFlags {
    bool empty=false, below=false, reached=false, effect=false;
};
struct StormboltPowerCellRemoteDecision {
    std::uint32_t count=0;
    bool continue_effects=false;
    bool reached_effect=false;
};
// PS2C4C577..2C4C6B2: count classification precedes edge detection.
inline StormboltPowerCellRemoteDecision decide_stormbolt_power_cell_remote(
    StormboltPowerCellRemoteFlags current, bool previous_effect,
    bool owner_enabled, std::uint32_t count, std::uint32_t threshold,
    bool associated_present, bool associated_player, bool associated_local) noexcept {
    if (!owner_enabled) {
        if (current.empty) count=0;
        else if (current.below) count=threshold-1u;
        else if (current.reached) count=threshold;
    }
    const bool effects=current.effect && !previous_effect && associated_present
        && associated_player && !associated_local;
    return {count,effects,effects && count==threshold};
}
} // namespace stormslinger
