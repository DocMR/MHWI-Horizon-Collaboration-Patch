#pragma once
#include "stormbolt_stage_core.h"

namespace stormslinger {
// PS4997516..7557: preserve the latched byte if the timer is not positive.
// Crossing zero does NOT clear the latch or clamp the remaining time.
inline std::uint8_t advance_stormbolt_input_window(StormboltStageState& state,float step) noexcept {
    auto held=state.input_latch;
    if (held && state.input_timer>0.0f) {
        state.input_timer-=step;
        state.input_latch=held=1;
    }
    return held; // PS SIL, potentially retained across the later input path.
}
// PS4998141..8161, only after the selected input/effect path. The caller passes
// the retained SIL or its native post-effect reread, not an invented fresh flag.
inline void activate_stormbolt_input_window(StormboltStageState& state,std::uint8_t held) noexcept {
    state.input_effect_active=1;
    if (!held) {
        state.input_timer=0.1f; // Exact float bits3DCCCCCD.
        state.input_latch=1;
    }
}
} // namespace stormslinger
