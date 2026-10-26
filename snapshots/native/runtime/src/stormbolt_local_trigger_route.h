#pragma once
#include "stormbolt_input_trigger.h"
#include "stormbolt_input_window.h"
#include "stormbolt_remote_effect.h"
#include "stormbolt_effect_fade.h"

namespace stormslinger {
// Selected PS7F90 route only, after local input-window advancement and action
// filtering. retained_held is SIL from that earlier advancement, not a fresh read.
template<std::size_t Capacity>
bool stormbolt_run_local_trigger_route(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,std::uint8_t retained_held,
    const StormboltInputTriggerApi& input,std::uint32_t (*attachment)(void*),
    const StormboltDecayEffectApi& effect,void (*actor_flag)(void*,std::uint32_t),
    std::uint32_t base_color,const StormboltEffectColorApi& color,
    const StormboltMaterialColorApi& material,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa) {
    StormboltStageState captured{},current{};
    if(!store.snapshot(token,captured)) return false;
    bool permitted=false;
    if(!stormbolt_read_input_trigger(initial_actor,input,permitted) || !store.snapshot(token,current)) return false;
    auto* weapon=reinterpret_cast<void*>(token.address);
    bool triggered=false;
    if(permitted && !store.transact(token,[&](auto& state) noexcept {
        float resource{},step{};
        stormbolt_read_input_resource(weapon,resource);
        std::memcpy(&step,static_cast<const std::byte*>(weapon)+0x68,4);
        triggered=stormbolt_advance_input_trigger(state,{true,false,false,resource},step);
        return true;
    })) return false;
    if(triggered) {
        if(!attachment) return false;
        void* component{};
        std::memcpy(&component,static_cast<const std::byte*>(initial_actor)+0x8808,sizeof(component));
        const auto parameter=attachment(weapon);
        if(!store.snapshot(token,current)) return false;
        bool created=false;
        if(!stormbolt_update_remote_effect(component,captured.counter,parameter,effect,&created) ||
           !store.snapshot(token,current)) return false;
        // PS813A rereads only after request/assignment. An already-active bank
        // jumps directly to8141 with the retained SIL.
        if(!store.transact(token,[&](auto& state) noexcept {
            activate_stormbolt_input_window(state,created ? state.input_latch : retained_held);
            return true;
        })) return false;
    }
    // PS8161: even rejected input continues through the actor flag/fade tail.
    if(!store.snapshot(token,current)) return false;
    if(current.counter<=8) {
        void* owner{};
        std::memcpy(&owner,static_cast<const std::byte*>(weapon)+0x9B0,sizeof(owner));
        if(!owner || !actor_flag) return false;
        actor_flag(owner,0x16C); // PS8714 bit12 and mirrored8745 bit4.
        if(!store.snapshot(token,current)) return false;
    }
    if(!stormbolt_update_effect_fade(store,token,current.input_effect_active!=0,base_color,color)) return false;
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
