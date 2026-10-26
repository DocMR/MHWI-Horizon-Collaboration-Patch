#pragma once
#include "stormbolt_power_cell_loop_state.h"

namespace stormslinger {
// PS5805728..5734: phase table [1000,400], active-timer table [400,700].
inline float stormbolt_power_cell_visual_scale(
    const StormboltPowerCellLoopState& state,bool thresholdEqual) noexcept {
    if (thresholdEqual) return state.timer>0.0f ? 700.0f : 400.0f;
    return state.visual_phase==0 ? 400.0f : 1000.0f;
}
// PS2E33D54/5F: invoked only after count == threshold.
inline void reset_stormbolt_power_cell_threshold_visual(StormboltPowerCellLoopState& state) noexcept {
    state.visual_phase=0;
    state.visual_timer=-1.0f;
}
// Both non-equal bands: PS33FD8/34011 and 343EB/3442A mirror
// PS34196/341C8 and 34407/345AB. Only their later effect vectors differ.
inline void update_stormbolt_power_cell_non_threshold_visual(
    StormboltPowerCellLoopState& state,float nativeStep) noexcept {
    if (state.visual_phase==0) {
        if (state.growth) { state.visual_phase=1;state.visual_timer=0.7f; }
        return;
    }
    if (state.visual_phase!=1) return;
    if (state.growth) { state.visual_timer=0.7f;return; }
    if (!(state.visual_timer>0.0f)) return;
    state.visual_timer=state.visual_timer-nativeStep;
    if (state.visual_timer<=0.0f) state.visual_phase=0;
}
} // namespace stormslinger
