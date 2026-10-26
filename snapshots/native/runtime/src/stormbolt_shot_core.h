#pragma once
#include "stormbolt_stage_core.h"
#include "generated_stormbolt_rows.h"

namespace stormslinger {
struct StormboltCoefficients { float context60 = 0, context6c = 0; };
// PS slot125 selects a list entry without clamping or changing the snapshot.
inline constexpr std::array<std::uint32_t, 9> ps_stormbolt_selectors{
    94, 94, 94, 95, 95, 96, 96, 96, 97};
inline std::uint32_t select_stormbolt_resource(const StormboltStageState& state,
    const std::array<std::uint32_t, 9>& selectors = ps_stormbolt_selectors,
    std::uint32_t out_of_range = 97) noexcept {
    return state.shot_snapshot < selectors.size()
        ? selectors[state.shot_snapshot] : out_of_range;
}
// PS slot124 applies only to ammo40 and clamps the stored snapshot itself.
inline bool prepare_stormbolt_coefficients(std::uint32_t ammo, StormboltStageState& state,
    const std::array<StormboltRawRow, 9>& rows, StormboltCoefficients& output) noexcept {
    if (ammo != 40) return false;
    const auto row = (std::min)(state.shot_snapshot, std::uint32_t{8});
    const auto a = rows[row].fields[5], b = rows[row].fields[6];
    if (!std::isfinite(a) || !std::isfinite(b)) return false;
    output = {a, b};
    state.shot_snapshot = row;
    return true;
}
// PS factory inner subtraction only; caller must validate its outer shot/owner guards.
inline bool subtract_stormbolt_energy(float& gauge, const StormboltStageState& state,
    const std::array<StormboltRawRow, 9>& rows) noexcept {
    if (state.shot_snapshot > 8 || !std::isfinite(gauge)) return false;
    const auto cost = rows[state.shot_snapshot].fields[2];
    if (!std::isfinite(cost)) return false;
    const auto remainder = gauge - cost;
    if (!std::isfinite(remainder)) return false;
    gauge = remainder <= 0 ? 0.0f : remainder;
    return true;
}
} // namespace stormslinger
