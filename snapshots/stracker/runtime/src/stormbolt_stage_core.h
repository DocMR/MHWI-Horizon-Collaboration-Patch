#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace stormslinger {
// Verified PS cWp13Param constructor2F2B3BD; units and later runtime overrides unbound.
inline constexpr float ps_post_shot_decay_default = 0.5f;
struct StormboltStageState {
    std::uint32_t counter = 0, shot_snapshot = 0;
    float action_timer = 0, decay_timer = 0;
    std::uint8_t effect_latch = 0; // PS weapon28A4; sidecar, never appended to PC object.
    float effect_alpha = 255.0f; // PS constructors4998D60/4998E40/4998EE0, field28A0.
    std::uint8_t input_effect_active = 0, input_latch = 0; // PS288C / PS2894.
    float attachment_blink_timer = -1.0f; // PS28A8, constructor4998E24.
    std::uint8_t attachment_blink_phase = 0; // PS28AC.
    float input_timer = 0; // PS2890; only meaningful after its latch is initialized.
    std::uintptr_t action_owner = 0;
    std::uint32_t action_phase = 0;
    std::uint8_t action_fired = 0;
};
struct StormboltStageTiming {
    std::array<float, 9> action_row{}, decay_row{};
    float post_shot_decay = 0;
};
// PS final-action reset39F2E10 only; caller must prove action+34 guard.
// This is NOT a generic weapon switch reset. The shot snapshot is preserved.
inline bool reset_stormbolt_final_action(StormboltStageState& state,
    const StormboltStageTiming& timing, bool action_reset_guard) noexcept {
    if (!action_reset_guard) return true;
    if (!std::isfinite(timing.action_row[0]) ||
        !std::isfinite(timing.post_shot_decay)) return false;
    auto next = state;
    next.counter = 0;
    next.action_timer = timing.action_row[0];
    next.decay_timer = timing.post_shot_decay;
    state = next;
    return true;
}
// Reference PS inner dataflow only. Caller must reproduce verified action guards.
inline bool advance_stormbolt_stage(StormboltStageState& state, const StormboltStageTiming& timing) noexcept {
    auto next = state;
    next.shot_snapshot = next.counter;
    ++next.counter; // PS DWORD increment, including unsigned wrap.
    if (next.counter <= 8) {
        if (!std::isfinite(timing.action_row[next.counter])) return false;
        next.action_timer = timing.action_row[next.counter];
    }
    if (!std::isfinite(timing.post_shot_decay)) return false;
    next.decay_timer = timing.post_shot_decay;
    state = next;
    return true;
}
// step_value is PS weapon+68; time units and outer update guards remain unbound.
inline bool decay_stormbolt_stage(StormboltStageState& state, const StormboltStageTiming& timing,
    float step_value) noexcept {
    if (!state.counter) return true;
    if (!std::isfinite(step_value) || !std::isfinite(state.decay_timer)) return false;
    auto next = state;
    next.decay_timer -= step_value;
    if (!std::isfinite(next.decay_timer)) return false;
    if (next.decay_timer <= 0) {
        next.counter = (std::min)(next.counter - 1, std::uint32_t{8});
        if (!std::isfinite(timing.decay_row[next.counter])) return false;
        next.decay_timer = timing.decay_row[next.counter];
    }
    state = next; // Exactly one decrement; no invented catch-up or negative remainder carry.
    return true;
}
inline std::uint32_t stormbolt_shot_row(const StormboltStageState& state) noexcept {
    return (std::min)(state.shot_snapshot, std::uint32_t{8});
}
} // namespace stormslinger
