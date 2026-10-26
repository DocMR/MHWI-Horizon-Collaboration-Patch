#pragma once
#include "stormbolt_power_cell_remote_adapter.h"
#include "stormbolt_power_cell_remote_cache.h"
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
// Flags are supplied by the transport mapping, not inferred from PS bit
// numbers on a PC packet. This reader authenticates the native state inputs.
template<std::size_t Capacity>
bool read_stormbolt_power_cell_remote_inputs(
    const StormboltPowerCellRemoteCache& cache, StormboltPowerCellRemoteFlags flags,
    bool previous_effect, StormboltPowerCellStore<Capacity>& statuses,
    const StormboltLoopInputBindings& bindings,
    StormboltPowerCellRemoteInputs& output) {
    std::uintptr_t status=0,associated=0,resource=0,vtable=0;
    std::uint8_t enabled=0,player=0;
    StormboltPowerCellRemoteInputs value{};
    if (!cache.actor || !read_stormbolt_field(cache.actor,0x7D20,status) || !status ||
        !read_stormbolt_field(status,0,vtable) || vtable!=bindings.status_vtable ||
        !statuses.token_for_live_address(status,value.status_token) ||
        !read_stormbolt_field(status,8,associated) ||
        !read_stormbolt_field(cache.actor,0x8938,enabled) || !bindings.resource_slot ||
        !read_stormbolt_field(bindings.resource_slot,0,resource) || !resource ||
        !read_stormbolt_field(resource,0,vtable) || vtable!=bindings.resource_vtable ||
        !read_stormbolt_field(resource,0xA8C,value.threshold)) return false;
    if (associated && !read_stormbolt_field(associated,0x60,player)) return false;
    value.flags=flags;value.previous_effect=previous_effect;
    value.owner_enabled=(enabled&1)!=0;
    value.associated=reinterpret_cast<void*>(associated);
    value.associated_player=(player&1)!=0;
    output=value;
    return true;
}
template<std::size_t Capacity>
bool resolve_stormbolt_power_cell_remote_identity(
    StormboltPowerCellRemoteInputs& inputs,StormboltPowerCellStore<Capacity>& statuses,
    void** manager_slot,void* (*local_player)(void*)) {
    std::uintptr_t manager=0;
    if (!manager_slot || !local_player || !read_stormbolt_field(
        reinterpret_cast<std::uintptr_t>(manager_slot),0,manager) || !manager) return false;
    const auto local=local_player(reinterpret_cast<void*>(manager));
    StormboltPowerCellState state{};
    if (!statuses.snapshot(inputs.status_token,state)) return false;
    inputs.associated_local=local && local==inputs.associated;
    return true;
}
} // namespace stormslinger
