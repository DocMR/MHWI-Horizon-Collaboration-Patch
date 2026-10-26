#pragma once
#include "stormbolt_local_input_prefix.h"
#include "stormbolt_local_trigger_route.h"
#include "stormbolt_decay_route.h"

namespace stormslinger {
struct StormboltLocalUpdateApi {
    StormboltLocalInputApi input{};
    StormboltInputTriggerApi trigger{};
    std::uint32_t (*attachment)(void*)=nullptr;
    StormboltDecayEffectApi effect{};
    StormboltCleanupBankApi cleanup{};
    StormboltDecayOwnerApi owner{};
    void (*actor_flag)(void*,std::uint32_t)=nullptr;
    StormboltEffectColorApi color{};
    StormboltMaterialColorApi material{};
};
// Complete selected local branch from7516. Outer authority/mode/category and
// local-owner selection must already have run; this never calls ordinary update.
template<std::size_t Capacity>
bool stormbolt_run_local_update(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,std::uint32_t group,
    const StormboltLocalUpdateApi& api,std::uint32_t base_color,
    const std::array<float,4>& low_hlsa,const std::array<float,4>& high_hlsa) {
    StormboltLocalSelection selection{};
    if(!stormbolt_select_local_input(store,token,initial_actor,group,api.input,selection)) return false;
    if(selection.route==StormboltLocalRoute::trigger)
        return stormbolt_run_local_trigger_route(store,token,initial_actor,selection.held,
            api.trigger,api.attachment,api.effect,api.actor_flag,base_color,api.color,
            api.material,low_hlsa,high_hlsa);
    if(selection.route==StormboltLocalRoute::decay) {
        void* component{};
        std::memcpy(&component,static_cast<const std::byte*>(initial_actor)+0x8808,sizeof(component));
        const auto result=stormbolt_decay_prefix(store,token,component,api.cleanup,api.effect);
        if(result==StormboltDecayPrefixResult::invalidated) return false;
        if(result==StormboltDecayPrefixResult::continue_after_effect &&
           !stormbolt_cleanup_decay_latch(store,token,api.owner,api.cleanup)) return false;
        if(!stormbolt_reload_local_decay_action_timer(store,token)) return false;
    }
    // Fade-only skips the decay action-timer reload. Read rising after all native
    // cleanup calls, at the shared7E4A continuation, rather than before dispatch.
    StormboltStageState state{};
    if(!store.snapshot(token,state) ||
       !stormbolt_update_effect_fade(store,token,state.input_effect_active!=0,base_color,api.color)) return false;
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,api.material);
}
} // namespace stormslinger
