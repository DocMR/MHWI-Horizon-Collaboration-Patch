#pragma once
#include "stormbolt_loop_update_inputs.h"

namespace stormslinger {
struct StormboltClass48SkillBindings {
    std::uintptr_t status_vtable=0,resource_slot=0,resource_vtable=0;
    StormboltSkillValueQuery query=nullptr;
};
// Called after the original native multiplier helper. Its class48 route has
// no final per-class multiplier; all common native factors remain in base.
template<std::size_t S>
bool apply_stormbolt_class48_skill(void* status,std::uint32_t classification,float base,
    StormboltPowerCellStore<S>& statuses,const StormboltClass48SkillBindings& bindings,float& output) {
    if(classification!=48) {output=base;return true;}
    const auto address=reinterpret_cast<std::uintptr_t>(status);
    std::uintptr_t vtable=0,owner=0,perks=0;
    StormboltPowerCellToken token{};
    if(!bindings.query || !bindings.status_vtable ||
        !read_stormbolt_field(address,0,vtable) || vtable!=bindings.status_vtable ||
        !statuses.token_for_live_address(address,token) ||
        !read_stormbolt_field(address,8,owner)) return false;
    // PS signed word parameter0 -> float -> DIVSS100, not native float query MULSS.01.
    const auto percent=bindings.query(reinterpret_cast<void*>(owner),70,0,100);
    StormboltPowerCellState live{};
    if(!statuses.snapshot(token,live) || !read_stormbolt_field(address,0x18,perks)) return false;
    float factor=static_cast<float>(percent)/100.0f;
    bool matching=false;
    if(perks) {
        std::array<std::uint32_t,3> ids{};
        if(!read_stormbolt_field(perks,0x1C,ids)) return false;
        for(auto id:ids) matching=matching || id==22;
    }
    if(matching) {
        std::uintptr_t resource=0;float set_factor=0;
        if(!bindings.resource_slot || !bindings.resource_vtable ||
            !read_stormbolt_field(bindings.resource_slot,0,resource) ||
            !read_stormbolt_field(resource,0,vtable) || vtable!=bindings.resource_vtable ||
            !read_stormbolt_field(resource,0xBD4,set_factor)) return false;
        factor=factor*set_factor;
    }
    output=base*factor;return true;
}
} // namespace stormslinger
