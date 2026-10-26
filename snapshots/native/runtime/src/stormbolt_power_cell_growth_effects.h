#pragma once
#include "stormbolt_power_cell_growth_commit.h"
#include "stormbolt_power_cell_prompt.h"
#include "stormbolt_loop_audio_update.h"

namespace stormslinger {
// Growth has already committed. Missing native combine component is a valid
// native skip; no state is created and committed count is never rolled back.
template<std::size_t Capacity>
bool apply_stormbolt_power_cell_growth_effects(StormboltPowerCellGrowthResult result,
    void* associated,StormboltPowerCellLoopStore<Capacity>& loops,
    const StormboltLoopAudioUpdateBindings& bindings,StormboltSkillPrompt prompt,
    bool show_local_prompt=true) {
    if (result==StormboltPowerCellGrowthResult::unchanged) return true;
    if (!associated) return false;
    if (show_local_prompt && result==StormboltPowerCellGrowthResult::reached_threshold &&
        !request_stormbolt_power_cell_prompt(associated,false,prompt)) return false;
    std::uintptr_t component=0,vtable=0;
    if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(associated),0x126C8,component)) return false;
    if (!component) return true;
    StormboltPowerCellLoopToken token{};
    if (!read_stormbolt_field(component,0,vtable) || vtable!=bindings.component_vtable ||
        !loops.token_for_live_address(component,token)) return false;
    if (result==StormboltPowerCellGrowthResult::below_threshold)
        return loops.transact(token,[](auto& state) noexcept {state.growth=true;return true;});
    if (!loops.transact(token,[](auto& state) noexcept {state.timer=5;return true;})) return false;
    StormboltLoopNativeContext context{};
    if (!read_stormbolt_loop_native_context(reinterpret_cast<void*>(component),bindings.component_vtable,
        bindings.container_at,bindings.native_first,bindings.native_second,context)) return false;
    if (!context.actor_available) return true;
    if (!bindings.request) return false;
    (void)request_stormbolt_power_cell_loop_play(context.container,context.actor,context.defaults,bindings.request);
    return loops.transact(token,[](auto& state) noexcept {state.active=true;return true;});
}
} // namespace stormslinger
