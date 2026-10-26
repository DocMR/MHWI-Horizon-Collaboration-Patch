#pragma once
#include "stormbolt_power_cell_loop_adapter.h"
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
struct StormboltLoopAudioUpdateBindings {
    std::uintptr_t component_vtable=0;
    void* (*container_at)(void*)=nullptr;
    const float* native_first=nullptr;
    const float* native_second=nullptr;
    StormboltLoopInputBindings inputs{};
    StormboltSkillValueQuery query=nullptr;
    StormboltNamedEventRequest request=nullptr;
};
// Audio portion only, invoked at the authenticated synchronous component
// update boundary. The complete PS visual branch is not implemented here.
template<std::size_t LoopCapacity,std::size_t StatusCapacity>
bool update_stormbolt_loop_audio_from_component(
    void* component,StormboltPowerCellLoopStore<LoopCapacity>& loops,
    StormboltPowerCellStore<StatusCapacity>& statuses,
    const StormboltLoopAudioUpdateBindings& bindings) {
    StormboltPowerCellLoopToken token{};
    if (!loops.token_for_live_address(reinterpret_cast<std::uintptr_t>(component),token))
        return false;
    StormboltLoopNativeContext context{};
    if (!read_stormbolt_loop_native_context(component,bindings.component_vtable,
        bindings.container_at,bindings.native_first,bindings.native_second,context)) return false;
    StormboltPowerCellLoopUpdateInputs input{};
    bool targetInactive=false;
    if (!read_stormbolt_loop_update_inputs(context.owner,statuses,bindings.inputs,bindings.query,input,&targetInactive)) {
        if(!targetInactive) return false;
        return process_stormbolt_power_cell_loop_audio(loops,token,context,{},bindings.request);
    }
    return process_stormbolt_power_cell_loop_audio(loops,token,context,input,bindings.request);
}
} // namespace stormslinger
