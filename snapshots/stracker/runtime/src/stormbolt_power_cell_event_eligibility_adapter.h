#pragma once
#include "stormbolt_power_cell_event_eligibility.h"
#include "stormbolt_loop_update_inputs.h"
#include "stormbolt_live_probe.h"

namespace stormslinger {
// Invoke at the status-update boundary corresponding to PS2C53024..F3.
// Status/owner lifetimes are borrowed synchronously. Native flags run outside
// the store lock and precede publishing the sidecar A14 value.
template<std::size_t Capacity>
bool update_stormbolt_power_cell_event_eligibility_from_status(void* status,
    StormboltPowerCellStore<Capacity>& store,const StormboltLoopInputBindings& bindings,
    StormboltSkillValueQuery query,void (*actorFlag)(void*,std::uint32_t)) {
    stormbolt_live_probe.status_calls.fetch_add(1,std::memory_order_relaxed);
    const auto address=reinterpret_cast<std::uintptr_t>(status);
    StormboltPowerCellToken token{};
    std::uintptr_t vtable=0,owner=0,ownerStatus=0;
    if (!store.token_for_live_address(address,token) ||
        !read_stormbolt_field(address,0,vtable) || vtable!=bindings.status_vtable ||
        !read_stormbolt_field(address,8,owner) || !owner ||
        !read_stormbolt_field(owner,0x7D20,ownerStatus) || ownerStatus!=address) return false;
    StormboltPowerCellLoopUpdateInputs input{};
    if (!read_stormbolt_loop_update_inputs(reinterpret_cast<void*>(owner),store,bindings,query,input)) return false;
    stormbolt_live_probe.status_inputs.fetch_add(1,std::memory_order_relaxed);
    std::uintptr_t weapon=0;
    std::uint32_t part8=0;
    if(read_stormbolt_field(owner,0x76B0,weapon)) (void)read_stormbolt_field(weapon,0x2388,part8);
    stormbolt_live_probe.status_part8.store(part8,std::memory_order_relaxed);
    stormbolt_live_probe.status_threshold.store(input.threshold,std::memory_order_relaxed);
    stormbolt_live_probe.status_skill_count.store(static_cast<std::uint32_t>(input.resource_count),std::memory_order_relaxed);
    StormboltPowerCellState snapshot{};
    if (!store.snapshot(token,snapshot)) return false;
    stormbolt_live_probe.status_count.store(snapshot.count,std::memory_order_relaxed);
    update_stormbolt_power_cell_event_eligibility(snapshot,input.owner_flag,
        input.resource_index_present,input.resource_count,input.threshold);
    if (snapshot.event_eligible) {
        stormbolt_live_probe.status_eligible.fetch_add(1,std::memory_order_relaxed);
        if (!actorFlag) return false;
        stormbolt_live_probe.status_flag_calls.fetch_add(1,std::memory_order_relaxed);
        actorFlag(reinterpret_cast<void*>(owner),0x98);
    }
    return store.transact(token,[&](auto& state) noexcept {
        if (state.count!=snapshot.count) return false;
        state.event_eligible=snapshot.event_eligible;
        return true;
    });
}
} // namespace stormslinger
