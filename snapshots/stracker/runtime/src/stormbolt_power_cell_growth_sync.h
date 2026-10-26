#pragma once
#include "stormbolt_power_cell_sync_store.h"
#include "stormbolt_power_cell_event_growth.h"

namespace stormslinger {
// PS2CEE5E7..652, after combine/prompt/PLAY: re-query the local actor and
// read the captured associated actor's current controller. No native call
// occurs under the sync store lock.
template<std::size_t Capacity>
bool mark_stormbolt_power_cell_sync_after_growth(
    StormboltPowerCellGrowthResult result,void* associated,
    void** manager_slot,void* (*local_player)(void*),std::uintptr_t master_vtable,
    StormboltPowerCellSyncStore<Capacity>& events) {
    if (result==StormboltPowerCellGrowthResult::unchanged) return true;
    std::uintptr_t manager=0,controller=0;
    if (!associated || !manager_slot || !local_player || !read_stormbolt_field(
        reinterpret_cast<std::uintptr_t>(manager_slot),0,manager) || !manager) return false;
    if (local_player(reinterpret_cast<void*>(manager))!=associated) return true;
    if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(associated),0x14F8,controller)) return false;
    if (!controller) return true;
    StormboltPowerCellSyncToken token{};
    if (!capture_stormbolt_power_cell_sync_controller(associated,master_vtable,events,token)) return false;
    return mark_stormbolt_power_cell_sync_controller(token,events);
}
} // namespace stormslinger
