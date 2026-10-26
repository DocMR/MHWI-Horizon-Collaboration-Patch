#pragma once
#include "stormbolt_stage_core.h"

namespace stormslinger {
// PS4997109..7174 / 49973AE..73D6, only after attachment prerequisites.
// Returns the RGB multiplier; color conversion and native material write follow.
inline float advance_stormbolt_attachment_blink(StormboltStageState& state,float step) noexcept {
    if (state.counter<6) {
        state.attachment_blink_phase=0;
        state.attachment_blink_timer=-1.0f;
        return 20.0f; // PS527B008.
    }
    state.attachment_blink_timer-=step;
    if (state.attachment_blink_timer<0.0f) {
        state.attachment_blink_phase^=1; // Native byte XOR, not Boolean inversion.
        state.attachment_blink_timer=0.1f;
    }
    return state.attachment_blink_phase ? 100.0f : 1.0f; // PS58059C8.
}
} // namespace stormslinger
