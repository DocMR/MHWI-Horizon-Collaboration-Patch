#pragma once
#include <array>
#include <cstdint>

namespace stormslinger {
// PS 19DCCF7..19DD0EF, after the inherited ordinary-controller refresh.
// Node ownership stays with the native light-bowgun HUD controller. Call only
// during its live refresh, after the target weapon/part/context gates pass.
struct StormboltChargeHudNodes {
    void* ordinary_root{}; // controller+C8
    void* charge_root{};   // controller+F0
    void* charge_child{};  // controller+F8
    void* on_off{};        // controller+100
    std::array<void*, 9> bars{}; // controller+108..148
    std::array<void*, 2> selected_ammo{}; // controller+150,158
    std::array<void*, 8> segments{}; // controller+168..1A0
    void* animation_context{}; // controller+70
};
struct StormboltChargeHudApi {
    void (*set_hidden)(void*, bool){};
    void (*set_frame)(void*, float, std::uint8_t){};
    float (*frame)(void*){};
    std::uint32_t (*animation)(void*){};
    bool (*play)(void*, std::uint32_t, void*){}; // PC18C6210: play only if changed
};
struct StormboltChargeHudInput {
    // The original computes zero when the owner's configuration is unavailable.
    bool configuration_available{};
    float charge{};
    float maximum_charge{};
    bool special_ammo_selected{};
    bool weapon_drawn{};
};

inline void refresh_stormbolt_charge_hud(const StormboltChargeHudNodes& nodes,
    const StormboltChargeHudInput& input, const StormboltChargeHudApi& api) {
    const float previous = api.frame(nodes.bars[0]);
    float percent = 0.0f;
    if (input.configuration_available)
        percent = (input.charge / input.maximum_charge) * 100.0f;

    api.set_hidden(nodes.charge_root, false);
    api.set_hidden(nodes.ordinary_root, true);
    for (void* node : nodes.segments) api.set_frame(node, 2.0f, 1);
    api.set_frame(nodes.on_off, 0.0f, 1);
    const float selected = input.special_ammo_selected && input.weapon_drawn
        ? 0.0f : 1.0f;
    for (void* node : nodes.selected_ammo) api.set_frame(node, selected, 1);
    for (void* node : nodes.bars) api.set_frame(node, percent, 1);

    if (previous < 100.0f && percent == 100.0f) {
        api.play(nodes.charge_root, 0xF4249, nodes.animation_context);
        api.play(nodes.charge_child, 0xF4249, nodes.animation_context);
    } else if (previous > 0.0f && percent == 0.0f) {
        api.play(nodes.charge_root, 0xF4241, nodes.animation_context);
    } else {
        const auto animation = api.animation(nodes.charge_root);
        if (percent > previous) {
            if (animation != 0xF4241)
                api.play(nodes.charge_root, 0xF4244, nodes.animation_context);
        } else if (animation != 0xF4249) {
            api.play(nodes.charge_root, 1, nodes.animation_context);
        }
    }
}
} // namespace stormslinger
