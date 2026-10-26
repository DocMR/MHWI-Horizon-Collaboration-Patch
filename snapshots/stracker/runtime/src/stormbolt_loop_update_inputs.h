#pragma once
#include "stormbolt_loop_skill_gate.h"
#include "stormbolt_power_cell_store.h"

namespace stormslinger {
struct StormboltLoopInputBindings {
    std::uintptr_t skill_vtable=0,status_vtable=0;
    std::uintptr_t resource_slot=0,resource_vtable=0;
    std::uintptr_t target_weapon_vtable=0;
};
template<std::size_t Capacity>
bool read_stormbolt_loop_update_inputs(
    void* owner,StormboltPowerCellStore<Capacity>& statusStore,
    const StormboltLoopInputBindings& bindings,StormboltSkillValueQuery query,
    StormboltPowerCellLoopUpdateInputs& output,bool* targetInactive=nullptr) {
    if(targetInactive) *targetInactive=false;
    // Production Power Cell additions belong only to the target bowguns.
    // Native status managers exist for every weapon; tracking one is not a
    // target-weapon authorization. Zero is reserved for standalone adapters.
    if(bindings.target_weapon_vtable) {
        const auto actor=reinterpret_cast<std::uintptr_t>(owner);
        std::uintptr_t weapon=0,table=0,backlink=0;
        std::uint32_t category=0,id=0;
        if(!read_stormbolt_field(actor,0x76B0,weapon)) return false;
        const auto inactive=[&]() {if(targetInactive) *targetInactive=true;return false;};
        if(!weapon) return inactive();
        if(!read_stormbolt_field(weapon,0,table)) return false;
        if(table!=bindings.target_weapon_vtable) return inactive();
        if(!read_stormbolt_field(weapon,0x9F8,category) ||
            !read_stormbolt_field(weapon,0x9FC,id)) return false;
        if(category!=13 || (id!=238 && id!=239)) return inactive();
        if(!read_stormbolt_field(weapon,0x9B0,backlink) || backlink!=actor) return false;
    }
    StormboltPowerCellLoopUpdateInputs candidate{};
    if (!read_stormbolt_loop_skill_gate(owner,bindings.skill_vtable,query,candidate)) return false;
    if (candidate.owner_flag && candidate.resource_index_present && candidate.resource_count>0) {
        std::uintptr_t status=0,vtable=0,resource=0;
        if (!bindings.status_vtable || !bindings.resource_vtable || !bindings.resource_slot ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(owner),0x7D20,status) ||
            !read_stormbolt_field(status,0,vtable) || vtable!=bindings.status_vtable) return false;
        StormboltPowerCellToken token{};
        StormboltPowerCellState state{};
        if (!statusStore.token_for_live_address(status,token) || !statusStore.snapshot(token,state) ||
            !read_stormbolt_field(bindings.resource_slot,0,resource) ||
            !read_stormbolt_field(resource,0,vtable) || vtable!=bindings.resource_vtable ||
            !read_stormbolt_field(resource,0xA8C,candidate.threshold) ||
            !read_stormbolt_field(resource,0xA90,candidate.related_count)) return false;
        candidate.count=state.count;
    }
    output=candidate;
    return true;
}
} // namespace stormslinger
