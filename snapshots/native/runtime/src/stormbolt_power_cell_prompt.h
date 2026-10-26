#pragma once
#include <cstdint>

namespace stormslinger {
using StormboltSkillPrompt=void (*)(void*,std::uint32_t,bool,std::int32_t);
// Native19A2D20 retains local-player filtering, localized skill-name/template
// lookup, formatting and queue dispatch. Caller guarantees actor/API/resource
// lifetimes and UI readiness. Invoked does not mean a visible prompt was shown.
inline bool request_stormbolt_power_cell_prompt(
    void* actor, bool removal, StormboltSkillPrompt request) {
    if (!actor || !request) return false;
    request(actor,0x1D3,removal,-1);
    return true;
}
} // namespace stormslinger
