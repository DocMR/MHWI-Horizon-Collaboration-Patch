#pragma once
#include "stormbolt_power_cell_timer.h"
#include "stormbolt_power_cell_prompt.h"
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
// Native status boundary after qualification update. nativeStep is the cached
// update step (PC XMM9), not a new read or an assumed number of seconds.
template<std::size_t Capacity>
bool advance_stormbolt_power_cell_timer_from_status(void* status,float nativeStep,
    StormboltPowerCellStore<Capacity>& store,const StormboltLoopInputBindings& bindings,
    StormboltSkillValueQuery query,StormboltSkillPrompt prompt) {
    const auto address=reinterpret_cast<std::uintptr_t>(status);
    StormboltPowerCellToken token{};
    std::uintptr_t vtable=0,owner=0;
    std::uint8_t flags=0;
    if (!store.token_for_live_address(address,token) ||
        !read_stormbolt_field(address,0,vtable) || vtable!=bindings.status_vtable ||
        !read_stormbolt_field(address,8,owner) || !owner ||
        !read_stormbolt_field(owner,0x8938,flags)) return false;
    StormboltPowerCellTimerResult result=StormboltPowerCellTimerResult::inactive;
    if (!store.transact(token,[&](auto& state) noexcept {
        result=advance_stormbolt_power_cell_timer(state.count,state.timer,(flags&1)!=0,nativeStep);
        return true;
    })) return false;
    if (result!=StormboltPowerCellTimerResult::expired) return true;
    // PS2C55B28 clears count/timer before either skill query or UI request.
    StormboltPowerCellLoopUpdateInputs skill{};
    if (!read_stormbolt_loop_skill_gate(reinterpret_cast<void*>(owner),bindings.skill_vtable,query,skill)) return false;
    if (!skill.owner_flag || !skill.resource_index_present || skill.resource_count<=0) return true;
    return request_stormbolt_power_cell_prompt(reinterpret_cast<void*>(owner),true,prompt);
}
} // namespace stormslinger
