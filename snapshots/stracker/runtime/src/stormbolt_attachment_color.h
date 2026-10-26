#pragma once
#include <array>
#include <immintrin.h>

namespace stormslinger {
// PS4999143/915B and 9177/9191 initialization. H,L,S,A; hue uses a six-unit circle.
inline constexpr std::array<float,4> stormbolt_attachment_low_hlsa{3.473f,0.5f,1.0f,1.0f};
inline constexpr std::array<float,4> stormbolt_attachment_high_hlsa{4.6f,0.6f,1.0f,1.0f};
// PS4A0E050: single wrap on R/B only, no normalization/clamping of the input.
inline std::array<float,4> stormbolt_attachment_rgba(const std::array<float,4>& hlsa,float multiplier) noexcept {
    const auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
    const float h=hlsa[0],l=hlsa[1],s=hlsa[2];
    std::array<float,4> rgba{l,l,l,hlsa[3]};
    if (s!=0.0f) {
        const float q=0.5f>=l ? mul(l,add(s,1.0f)) : sub(add(l,s),mul(l,s));
        const float p=sub(add(l,l),q);
        const auto wrap=[&](float v){if(v>=6.0f)v=add(v,-6.0f);if(v<0.0f)v=add(v,6.0f);return v;};
        const auto channel=[&](float v){
            if(v<1.0f)return add(p,mul(sub(q,p),v));
            if(v<3.0f)return q;
            if(v<4.0f)return add(p,mul(sub(q,p),sub(4.0f,v)));
            return p;
        };
        rgba[0]=channel(wrap(add(h,2.0f)));
        rgba[1]=channel(h);
        rgba[2]=channel(wrap(add(h,-2.0f)));
    }
    for(unsigned i=0;i<3;++i)rgba[i]=mul(multiplier,rgba[i]);
    return rgba; // Alpha is copied, never multiplied.
}
} // namespace stormslinger
