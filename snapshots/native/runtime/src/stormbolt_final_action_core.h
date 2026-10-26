#pragma once
#include "stormbolt_stage_core.h"

namespace stormslinger {
struct StormboltFinalActionState {
    std::uint32_t phase = 0;
    bool firing_call_returned = false;
};
enum class StormboltFinalStep { invalid, waiting, armed, firing_requested };
// PS39F2C00 local pre-fire decision only. Inputs are captured native values;
// actor flags, origin selection, firing and termination remain caller obligations.
// This does not replace the complete native action or determine success of a shot.
inline StormboltFinalStep prepare_stormbolt_final_step(
    StormboltFinalActionState& action, StormboltStageState& weapon,
    std::uint32_t& event_flags, float native_value_a, float native_value_b,
    const StormboltStageTiming& timing) noexcept {
    if (action.phase == 0) {
        if (!std::isfinite(native_value_a) || !std::isfinite(native_value_b))
            return StormboltFinalStep::invalid;
        const float difference = native_value_a - native_value_b;
        if (!std::isfinite(difference)) return StormboltFinalStep::invalid;
        if (std::fabs(difference) > 30.0f) return StormboltFinalStep::waiting;
        action.phase = 1;
        return StormboltFinalStep::armed; // PS does not fall through to fire this tick.
    }
    if (action.phase != 1 || !(event_flags & 1u)) return StormboltFinalStep::waiting;
    auto candidate = weapon;
    if (!advance_stormbolt_stage(candidate, timing)) return StormboltFinalStep::invalid;
    event_flags &= ~std::uint32_t{1}; // Preserve all other event bits.
    weapon = candidate; // PS stage update precedes native firing call.
    return StormboltFinalStep::firing_requested;
}
// Invoke only after the native firing call returns; not proof a shell was created.
inline void mark_stormbolt_final_firing_returned(StormboltFinalActionState& action) noexcept {
    action.firing_call_returned = true;
}
struct StormboltFinalActorFlags {
    std::uint32_t ps8714 = 0;
    std::uint8_t ps8745 = 0;
};
// PS39F2D41..39F2DE5 decisions on captured values only. Field labels are PS
// evidence, NOT PC offsets. The caller still performs native follow-up effect
// creation/binding and always delegates the final completion query.
inline bool prepare_stormbolt_final_post_step(const StormboltFinalActionState& action,
    std::uint32_t& event_flags, StormboltFinalActorFlags& actor_flags,
    const std::array<bool, 4>& native_followup_references_active) noexcept {
    if (!action.firing_call_returned) {
        actor_flags.ps8714 |= 0x2000u;
        actor_flags.ps8745 |= 0x20u;
    }
    if (!(event_flags & 0x40000000u)) return false;
    event_flags &= ~std::uint32_t{0x40000000};
    // PS consumes bit30 even when an active reference suppresses creation.
    for (bool active : native_followup_references_active)
        if (active) return false;
    return true; // Request native follow-up; not completion or creation success.
}
} // namespace stormslinger
