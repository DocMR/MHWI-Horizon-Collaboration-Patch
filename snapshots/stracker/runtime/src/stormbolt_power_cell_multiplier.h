#pragma once
#include <cstdint>

namespace stormslinger {
// PS2C65A22..2C65A44. Caller already proved skill eligibility and chooses the
// multiplier at the original arithmetic position, not after all native math.
inline std::uint32_t select_stormbolt_power_cell_multiplier(
    std::uint32_t count, std::uint32_t threshold, std::uint32_t related_count) noexcept {
    if (count==threshold) return 2;
    const std::uint32_t boundary=threshold-related_count;
    return count>=boundary ? 1u : 0u;
}
} // namespace stormslinger
