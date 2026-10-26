#pragma once
#include <cstdint>

namespace stormslinger {
struct StormboltSelfDamageBranch {
    bool conditional;
    bool raise_state_and_notify;
};
// PS4998500..499854F and4998ACB..4998AE7 only. Inputs are values
// already read from validated native objects; this does not map PC fields.
// table_value is signed int16. state/threshold use unsigned comparisons.
inline StormboltSelfDamageBranch stormbolt_self_damage_branch(
    std::uint32_t snapshot,bool associated_actor_present,bool actor_flag,
    std::int32_t table_index,std::int16_t table_value,
    std::uint32_t state,std::uint32_t threshold) noexcept {
    const bool conditional=associated_actor_present && actor_flag && table_index!=0 &&
        table_value>0 && state!=threshold;
    return {conditional,conditional && snapshot>=8 && state<threshold};
}
} // namespace stormslinger
