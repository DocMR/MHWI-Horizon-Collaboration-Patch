#pragma once
#include "stormbolt_power_cell_sync_store.h"
#include "stormbolt_power_cell_sync_classification.h"
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
struct StormboltPowerCellSyncSnapshot {
    StormboltPowerCellSyncToken controller_token{};
    StormboltPowerCellToken status_token{};
    StormboltPowerCellRemoteFlags flags{};
};
template<std::size_t ControllerCapacity,std::size_t StatusCapacity>
bool read_stormbolt_power_cell_sync_snapshot(void* controller,std::uintptr_t master_vtable,
    StormboltPowerCellSyncStore<ControllerCapacity>& events,
    StormboltPowerCellStore<StatusCapacity>& statuses,
    const StormboltLoopInputBindings& bindings,StormboltSkillValueQuery query,
    StormboltPowerCellSyncSnapshot& output) {
    const auto address=reinterpret_cast<std::uintptr_t>(controller);
    std::uintptr_t owner=0,status=0,resource=0,vtable=0;
    StormboltPowerCellSyncSnapshot value{};
    if (!controller || !master_vtable || !events.token_for_live_address(address,value.controller_token) ||
        !read_stormbolt_field(address,0,vtable) || vtable!=master_vtable ||
        !read_stormbolt_field(address,0xDB0,owner) || !owner ||
        !read_stormbolt_field(owner,0x7D20,status) || !status ||
        !read_stormbolt_field(status,0,vtable) || vtable!=bindings.status_vtable ||
        !statuses.token_for_live_address(status,value.status_token)) return false;
    StormboltPowerCellLoopUpdateInputs skill{};
    if (!read_stormbolt_loop_skill_gate(reinterpret_cast<void*>(owner),bindings.skill_vtable,query,skill)) return false;
    std::uint32_t threshold=0,window=0;
    if (!bindings.resource_slot || !read_stormbolt_field(bindings.resource_slot,0,resource) || !resource ||
        !read_stormbolt_field(resource,0,vtable) || vtable!=bindings.resource_vtable ||
        !read_stormbolt_field(resource,0xA8C,threshold) || !read_stormbolt_field(resource,0xA90,window)) return false;
    StormboltPowerCellState state{};
    StormboltPowerCellSyncEventState event{};
    // Revalidate both captured generations after native skill queries.
    if (!statuses.snapshot(value.status_token,state) || !events.snapshot(value.controller_token,event)) return false;
    value.flags=classify_stormbolt_power_cell_sync(
        skill.owner_flag && skill.resource_index_present && skill.resource_count>0,
        state.count,threshold,window,event);
    output=value;return true;
}
} // namespace stormslinger
