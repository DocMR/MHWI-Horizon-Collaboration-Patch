#pragma once
#include "stormbolt_material_color.h"
#include "stormbolt_power_cell_visual_state.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
inline constexpr std::uint32_t stormbolt_power_cell_color_parameter_hash=0xA5FC8881u;
enum class StormboltPowerCellVisualBand { lower, upper, threshold };
using StormboltVisualConvert=float* (*)(const float*,float*);
using StormboltMaterialAt=void* (*)(void*,std::uint32_t);
using StormboltMaterialScalar=void (*)(void*,std::uint32_t,float);
// PS34877..3493A writes a single scalar after threshold color, before
// effect-vector work. Native PC23D8180 likewise writes only one DWORD.
inline bool stormbolt_write_power_cell_threshold_scalar(void* material,
    const StormboltPowerCellLoopState& state,StormboltMaterialScalar write) {
    if (!material || !write) return false;
    write(material,0xE8C621B1u,state.timer>0.0f?0.9f:0.08f);
    return true;
}
// PS3493F..34A39: this is a material vector, not an EFX spawn request.
// Runtime vectors6E29570/A0 are initialized from57ED300/330.
inline bool stormbolt_write_power_cell_threshold_vector(void* material,
    const StormboltPowerCellLoopState& state,const StormboltMaterialColorApi& api) {
    if (!material || !api.write_vector) return false;
    const std::array<float,4> value=state.timer>0.0f
        ? std::array<float,4>{0.0f,40.0f,20.0f,0.0f}
        : std::array<float,4>{0.0f,40.0f,150.0f,-12.5f};
    api.write_vector(material,0x58662D14u,value.data());
    return true;
}
// PS34C91..34D3A and34DE4..34E8D. No timer clamping in native code.
// Other phases use7BF6250, explicitly zeroed by4A2EC46 ->4A2ED20.
inline bool stormbolt_power_cell_non_threshold_vector(
    const StormboltPowerCellLoopState& state,std::array<float,4>& output) noexcept {
    if (state.visual_phase==0) { output={0.0f,40.0f,150.0f,-12.5f};return true; }
    if (state.visual_phase!=1) { output={0.0f,0.0f,0.0f,0.0f};return true; }
    const std::array<float,4> from{65.0f,40.0f,150.0f,-12.5f};
    const std::array<float,4> to{4.0f,40.0f,150.0f,-12.5f};
    const float ratio=state.visual_timer/0.7f;
    std::array<float,4> value{};
    for (unsigned i=0;i<4;++i) {
        const float difference=to[i]-from[i];
        const float scaled=ratio*difference;
        value[i]=from[i]+scaled;
    }
    output=value;
    return true;
}
inline bool stormbolt_write_power_cell_non_threshold_vector(void* material,
    const StormboltPowerCellLoopState& state,const StormboltMaterialColorApi& api) {
    if (!material || !api.write_vector) return false;
    std::array<float,4> value{};
    if (!stormbolt_power_cell_non_threshold_vector(state,value)) return false;
    api.write_vector(material,0x58662D14u,value.data());
    return true;
}
// PS33B02..33B60 retains the last material whose40E flag is zero.
// PC common update12EDD85/8A uses222F910 and the corresponding446 flag.
// Caller borrows owner and material-list lifetimes for this synchronous pass.
inline bool select_stormbolt_power_cell_material(void* owner,
    StormboltMaterialAt at,void*& output) {
    if (!owner || !at) return false;
    std::uint32_t count=0;
    if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(owner),0x2B8,count)) return false;
    void* selected=nullptr;
    for (std::uint32_t index=0;index<count;++index) {
        void* material=at(owner,index);
        std::uint8_t excluded=0;
        if (!material || !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(material),0x446,excluded)) return false;
        if (!excluded) selected=material;
    }
    output=selected;
    return true; // Empty/all-excluded lists legitimately select no material.
}
// The caller selects the native band and borrows the material lifetime.
// This dispatches the material parameter only; it does not acquire effects,
// advance the phase machine, or decide count/related-count branches.
inline bool stormbolt_write_power_cell_visual_color(void* material,
    StormboltPowerCellVisualBand band,const StormboltPowerCellLoopState& state,
    StormboltVisualConvert convert,const StormboltMaterialColorApi& api) {
    if (!material || !convert || !api.write_vector) return false;
    // PS2E2F5E0 initializes these from57ED2D0/E0/F0.
    std::array<float,4> source{3.7f,0.6f,1.0f,1.0f};
    switch (band) {
        case StormboltPowerCellVisualBand::lower: break;
        case StormboltPowerCellVisualBand::upper: source[0]=4.0f;break;
        case StormboltPowerCellVisualBand::threshold: source[0]=6.0f;break;
        default: return false;
    }
    std::array<float,4> output{};
    convert(source.data(),output.data()); // PC input first, output second.
    const float scale=stormbolt_power_cell_visual_scale(state,
        band==StormboltPowerCellVisualBand::threshold);
    for (unsigned channel=0;channel<3;++channel) output[channel]=output[channel]*scale;
    api.write_vector(material,stormbolt_power_cell_color_parameter_hash,output.data());
    return true; // Native missing-parameter no-op remains valid.
}
// Caller has already passed material/570 gates and reset threshold phase.
// All three native queries may independently no-op; retain their PS order.
inline bool stormbolt_write_power_cell_threshold_material(void* material,
    const StormboltPowerCellLoopState& state,StormboltVisualConvert convert,
    StormboltMaterialScalar scalar,const StormboltMaterialColorApi& api) {
    if (!material || !convert || !scalar || !api.write_vector) return false;
    stormbolt_write_power_cell_visual_color(material,
        StormboltPowerCellVisualBand::threshold,state,convert,api);
    stormbolt_write_power_cell_threshold_scalar(material,state,scalar);
    stormbolt_write_power_cell_threshold_vector(material,state,api);
    return true;
}
inline bool stormbolt_write_power_cell_non_threshold_material(void* material,
    StormboltPowerCellVisualBand band,const StormboltPowerCellLoopState& state,
    StormboltVisualConvert convert,StormboltMaterialScalar scalar,
    const StormboltMaterialColorApi& api) {
    if (!material || !convert || !scalar || !api.write_vector) return false;
    if (band!=StormboltPowerCellVisualBand::lower && band!=StormboltPowerCellVisualBand::upper) return false;
    stormbolt_write_power_cell_visual_color(material,band,state,convert,api);
    scalar(material,0xE8C621B1u,0.08f);
    stormbolt_write_power_cell_non_threshold_vector(material,state,api);
    return true;
}
} // namespace stormslinger
