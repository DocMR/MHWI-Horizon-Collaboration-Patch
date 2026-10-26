#pragma once
#include "stormbolt_stage_core.h"
#include <xmmintrin.h>

namespace stormslinger {
// PS4997F41..7F4A: truncating x86 conversion, then DWORD shift and RGB merge.
// Do not use a C++ float-to-int cast: NaN/out-of-range conversion is undefined
// there. The intrinsic follows native CVTTSS2SI, including masked invalid input.
inline std::uint32_t stormbolt_pack_effect_color(float alpha,std::uint32_t color) noexcept {
    const auto integer=static_cast<std::uint32_t>(_mm_cvttss_si32(_mm_set_ss(alpha)));
    return (integer<<24)|(color&0x00FFFFFFu);
}
// PS4998A44..8A5C only: caller already created/updated the selected effect.
// Repeated activation preserves the current fade value, including raw NaN bits.
inline void activate_stormbolt_effect_latch(StormboltStageState& state) noexcept {
    if (!state.effect_latch) state.effect_alpha=255.0f;
    state.effect_latch=1;
}
// PS4997E4A..7ED4 only. rising is the separately established PS288C flag.
// Keep scalar operations in source order; no fused multiply-add or symmetric
// clamp: rising clamps only above255, falling only below0. NaN remains unordered.
// Caller still performs the native effect lookup and packed-color write.
inline bool advance_stormbolt_effect_alpha(StormboltStageState& state,float step,bool rising) noexcept {
    if (!state.effect_latch) return false;
    auto delta=_mm_set_ss(step);
    if (rising) delta=_mm_add_ss(delta,delta);
    delta=_mm_mul_ss(delta,_mm_set_ss(rising ? 255.0f : -255.0f));
    const float value=_mm_cvtss_f32(_mm_add_ss(delta,_mm_set_ss(state.effect_alpha)));
    state.effect_alpha=rising ? (value>255.0f ? 255.0f : value) : (value<0.0f ? 0.0f : value);
    return true;
}
} // namespace stormslinger
