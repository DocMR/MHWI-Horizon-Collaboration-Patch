#pragma once
#include "stormbolt_stage_selection.h"
#include "stormbolt_local_update.h"
#include "stormbolt_inactive_update.h"
#include "stormbolt_cleanup_route.h"
#include "stormbolt_high_stage.h"

namespace stormslinger {
struct StormboltStageUpdateApi {
    StormboltUpdateGateApi gate{};
    StormboltActionDescriptor descriptor=nullptr;
    StormboltLocalUpdateApi routes{};
    const std::uint32_t* ammo_items=nullptr;
};
// Extension dispatch only. The caller must first run the original PC weapon
// update exactly once, then supply its retained actor and a live target token.
// The PS-only platform gate is not transplanted as an invented PC field.
template<std::size_t Capacity>
bool stormbolt_run_stage_update(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,const StormboltStageUpdateApi& api,
    std::uint32_t base_color,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa) {
    StormboltStageState state{};
    if(!store.snapshot(token,state)) return false;
    const auto gate=stormbolt_update_common_gate(reinterpret_cast<void*>(token.address),api.gate);
    if(gate==StormboltUpdateGateResult::unavailable || !store.snapshot(token,state)) return false;
    const auto& r=api.routes;
    if(gate==StormboltUpdateGateResult::inactive)
        return stormbolt_run_inactive_update(store,token,initial_actor,r.owner,api.ammo_items,
            r.material,low_hlsa,high_hlsa);
    StormboltStageSelection selection{};
    if(!stormbolt_read_stage_selection(store,token,initial_actor,api.descriptor,selection)) return false;
    if(selection.route==StormboltStageRoute::cleanup)
        return stormbolt_run_cleanup_route(store,token,initial_actor,r.cleanup,r.effect,r.owner,
            r.material,low_hlsa,high_hlsa);
    if(selection.route==StormboltStageRoute::local_input)
        return stormbolt_run_local_update(store,token,initial_actor,selection.group,r,
            base_color,low_hlsa,high_hlsa);
    void* component{};
    std::memcpy(&component,static_cast<const std::byte*>(initial_actor)+0x8808,sizeof(component));
    if(selection.route==StormboltStageRoute::high_stage)
        return stormbolt_run_high_stage(store,token,component,selection.local,r.cleanup,
            r.material,low_hlsa,high_hlsa);
    if(selection.route==StormboltStageRoute::remote_effect)
        return stormbolt_run_remote_effect(store,token,component,r.attachment,r.effect,
            r.material,low_hlsa,high_hlsa);
    return stormbolt_run_decay_with_attachment(store,token,component,false,false,base_color,
        r.cleanup,r.effect,r.owner,r.color,r.material,low_hlsa,high_hlsa);
}
} // namespace stormslinger
