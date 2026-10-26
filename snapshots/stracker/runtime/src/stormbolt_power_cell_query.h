#pragma once
#include <cstdint>

namespace stormslinger {
using StormboltSkillValueQuery = std::int32_t (*)(void*,std::uint32_t,
    std::uint32_t,std::int32_t);
struct StormboltPowerCellQueryResult {
    bool available;
    bool positive;
};
// Native1922190 validates actor/type flags and absent index, returning default0.
// The caller guarantees native actor/component/table lifetime and readiness.
// This only supplies the skill predicate; it does not establish the state gate.
inline StormboltPowerCellQueryResult stormbolt_power_cell_query(
    void* associated_actor,StormboltSkillValueQuery query) {
    if (!associated_actor) return {true,false};
    if (!query) return {false,false};
    return {true,query(associated_actor,0xD1,0,0)>0};
}
} // namespace stormslinger
