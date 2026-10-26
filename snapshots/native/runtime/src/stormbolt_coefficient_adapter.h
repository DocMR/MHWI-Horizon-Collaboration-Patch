#pragma once
#include "stormbolt_shot_core.h"
#include <cstring>

namespace stormslinger {
// Buffer adapter only. Native callers must separately prove object type,
// writable lifetime, weapon identity, synchronization and update-before-MOD order.
// No hook is installed by this header, and this is not a safe arbitrary-pointer API.
enum class CoefficientApplyResult { unrelated, invalid_target, invalid_parameters, applied };
inline CoefficientApplyResult apply_stormbolt_coefficients(
    std::uint32_t weapon_category, std::uint32_t equipment_id, std::uint32_t ammo,
    bool shell_info_type_confirmed,
    unsigned char* writable_buffer, std::size_t bytes, StormboltStageState& state,
    const std::array<StormboltRawRow, 9>& rows) noexcept {
    if (weapon_category != 13 || (equipment_id != 238 && equipment_id != 239) || ammo != 40)
        return CoefficientApplyResult::unrelated;
    if (!shell_info_type_confirmed || !writable_buffer || bytes < 0x6B0)
        return CoefficientApplyResult::invalid_target;
    auto candidate_state = state;
    StormboltCoefficients coefficients;
    if (!prepare_stormbolt_coefficients(ammo, candidate_state, rows, coefficients))
        return CoefficientApplyResult::invalid_parameters;
    // PS writes only these two DWORDs. Do not copy or clear the entire object.
    std::memcpy(writable_buffer + 0x60, &coefficients.context60, sizeof(float));
    std::memcpy(writable_buffer + 0x6C, &coefficients.context6c, sizeof(float));
    state = candidate_state;
    return CoefficientApplyResult::applied;
}
} // namespace stormslinger
