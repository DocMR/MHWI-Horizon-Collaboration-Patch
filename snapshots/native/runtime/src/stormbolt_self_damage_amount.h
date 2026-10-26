#pragma once
#include <array>
#include <cstdint>

namespace stormslinger {
struct StormboltSelfDamageAmounts {
    std::array<float,9> normal{};      // Parameter1614 +40*stage.
    std::array<float,9> conditional{}; // Parameter1618 +40*stage.
};
struct StormboltSelfDamageAmount {
    float value;
    bool call_adjustment;
};
// Only PS4998555/4998ACB/4998B39 amount selection and4998574 positive gate.
// Caller establishes the native conditional branch, snapshot clamp and owner
// lifetime. The stage8 conditional branch has additional native state writes
// and notifications before adjustment; this helper does NOT replace them.
inline StormboltSelfDamageAmount stormbolt_self_damage_amount(std::uint32_t snapshot,
    bool conditional,const StormboltSelfDamageAmounts& amounts,float health_field64) noexcept {
    const auto stage=snapshot<9 ? snapshot : 8;
    const float value=conditional ? (stage==8 ? health_field64*0.5f : amounts.conditional[stage])
                                  : amounts.normal[stage];
    return {value,value>0.0f}; // Ordered comparison: NaN, zero and negatives skip.
}
} // namespace stormslinger
