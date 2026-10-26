#pragma once
#include "stormbolt_loop_audio_update.h"
#include "stormbolt_power_cell_update.h"

namespace stormslinger {
struct StormboltLoopComponentUpdateBindings {
    StormboltLoopAudioUpdateBindings audio{};
    StormboltMaterialAt material_at=nullptr;
    StormboltVisualConvert convert=nullptr;
    StormboltMaterialScalar scalar=nullptr;
    StormboltMaterialColorApi material{};
};
template<std::size_t L,std::size_t S>
bool update_stormbolt_loop_from_component(void* component,
    StormboltPowerCellLoopStore<L>& loops,StormboltPowerCellStore<S>& statuses,
    const StormboltLoopAudioUpdateBindings& bindings) {
    return update_stormbolt_loop_audio_from_component(component,loops,statuses,bindings);
}
template<std::size_t L,std::size_t S>
bool update_stormbolt_loop_from_component(void* component,
    StormboltPowerCellLoopStore<L>& loops,StormboltPowerCellStore<S>& statuses,
    const StormboltLoopComponentUpdateBindings& bindings) {
    StormboltPowerCellLoopToken token{};
    if (!loops.token_for_live_address(reinterpret_cast<std::uintptr_t>(component),token)) return false;
    const auto& audio=bindings.audio;
    StormboltLoopNativeContext context{};
    if (!read_stormbolt_loop_native_context(component,audio.component_vtable,
        audio.container_at,audio.native_first,audio.native_second,context)) return false;
    StormboltPowerCellLoopUpdateInputs input{};
    bool targetInactive=false;
    if (!read_stormbolt_loop_update_inputs(context.owner,statuses,audio.inputs,audio.query,input,&targetInactive)) {
        // A valid equipment change may end a loop started by this patch. Do not
        // query/write non-target skills or materials; only stop an active loop.
        if(!targetInactive) return false;
        return process_stormbolt_power_cell_loop_audio(loops,token,context,{},audio.request);
    }
    void* material=nullptr;
    std::uint8_t disabled=0;
    if (input.owner_flag && input.resource_index_present && input.resource_count>0) {
        if (!select_stormbolt_power_cell_material(context.owner,bindings.material_at,material)) return false;
        // PS short-circuits570 when no material is selected.
        if (material && !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(component),0x570,disabled)) return false;
    }
    return update_stormbolt_power_cell_from_inputs(loops,token,context,input,material,disabled!=0,
        bindings.convert,bindings.scalar,bindings.material,audio.request);
}
} // namespace stormslinger
