#pragma once
#include "stormbolt_instance_store.h"
#include "stormbolt_power_cell_sync_event.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
struct StormboltPowerCellSyncToken {
    std::uintptr_t address=0;
    std::uint64_t generation=0;
    explicit operator bool() const noexcept {return address && generation;}
};
template<std::size_t Capacity>
using StormboltPowerCellSyncStore=StormboltInstanceStore<Capacity,
    StormboltPowerCellSyncEventState,StormboltPowerCellSyncToken>;
// PC actor14F8 -> Master controller; DB0 must point back to that actor.
template<std::size_t Capacity>
bool capture_stormbolt_power_cell_sync_controller(void* actor,std::uintptr_t master_vtable,
    StormboltPowerCellSyncStore<Capacity>& store,StormboltPowerCellSyncToken& output) {
    std::uintptr_t controller=0,vtable=0,owner=0;
    StormboltPowerCellSyncToken token{};
    if (!actor || !master_vtable || !read_stormbolt_field(
        reinterpret_cast<std::uintptr_t>(actor),0x14F8,controller) || !controller ||
        !read_stormbolt_field(controller,0,vtable) || vtable!=master_vtable ||
        !read_stormbolt_field(controller,0xDB0,owner) || owner!=reinterpret_cast<std::uintptr_t>(actor) ||
        !store.token_for_live_address(controller,token)) return false;
    output=token;return true;
}
template<std::size_t Capacity>
bool mark_stormbolt_power_cell_sync_controller(StormboltPowerCellSyncToken token,
    StormboltPowerCellSyncStore<Capacity>& store) {
    return store.transact(token,[](auto& state) noexcept {
        mark_stormbolt_power_cell_sync_event(state);return true;
    });
}
template<std::size_t Capacity>
bool complete_stormbolt_power_cell_sync_controller(StormboltPowerCellSyncToken token,
    StormboltPowerCellSyncStore<Capacity>& store) {
    return store.transact(token,[](auto& state) noexcept {
        complete_stormbolt_power_cell_sync_producer(state);return true;
    });
}
} // namespace stormslinger
