#pragma once
#include "stormbolt_update_gate.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltStageSelection {
    StormboltStageRoute route=StormboltStageRoute::cleanup;
    bool local=false;
    std::uint32_t group=0;
};
// Post-authority PS6F5A prefix only. initial_actor is retained by outer update.
template<std::size_t Capacity>
bool stormbolt_read_stage_selection(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,StormboltActionDescriptor descriptor,
    StormboltStageSelection& output) {
    StormboltStageState state{};
    if(!initial_actor || !store.snapshot(token,state)) return false;
    std::uint32_t flags{};
    std::memcpy(&flags,static_cast<const std::byte*>(initial_actor)+0x8938,4);
    StormboltStageSelection selected{};selected.local=(flags&1)!=0;
    if(selected.local && !store.transact(token,[](auto& next) noexcept {
        next.input_effect_active=0;return true;
    })) return false;
    std::uint8_t mode{},enabled{};
    std::memcpy(&mode,reinterpret_cast<const std::byte*>(token.address)+0x2350,1);
    std::uint64_t action=0;
    if(mode) {
        std::memcpy(&enabled,static_cast<const std::byte*>(initial_actor)+0x76A8,1);
        if(enabled) {
            if(!descriptor) return false;
            std::memcpy(&selected.group,static_cast<const std::byte*>(initial_actor)+0x6274,4);
            action=descriptor(initial_actor,0);
        }
    }
    if(!store.snapshot(token,state)) return false;
    // Pure selector may clear its local copy; the real sidecar was cleared
    // before native descriptor dispatch, never again after that callback.
    selected.route=stormbolt_select_stage_route(state,selected.local,mode!=0,enabled!=0,action);
    output=selected;
    return true;
}
} // namespace stormslinger
