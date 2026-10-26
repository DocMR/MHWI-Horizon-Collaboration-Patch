#pragma once
#include <cstdint>

namespace stormslinger {
// PS499886A..8887: uses current counter (player+2884), not shot snapshot.
// Caller must establish entry into this effect branch and bind its own provider.
constexpr std::uint32_t select_stormbolt_2011_ordinary_record(
    std::uint32_t current_counter) noexcept {
    const auto index = current_counter - std::uint32_t{3};
    return index <= 5 ? (index <= 2 ? 1u : 2u) : 0u;
}
// PS49987AE..87D9: terminal branch uses snapshot (player+2888) == 8.
constexpr bool stormbolt_2011_terminal_record_requested(
    std::uint32_t shot_snapshot) noexcept {
    return shot_snapshot == 8;
}
constexpr std::uint32_t ps_stormbolt_2011_terminal_record = 3;
// These are record selectors, not native calls or a global effect-ID replacement.
} // namespace stormslinger
