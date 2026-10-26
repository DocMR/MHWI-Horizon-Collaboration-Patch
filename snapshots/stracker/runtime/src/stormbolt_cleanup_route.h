#pragma once
#include "stormbolt_cleanup_decay.h"
#include "stormbolt_decay_effect.h"
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_attachment_update.h"

namespace stormslinger {
// Selected PS7179 cleanup branch. Actual decrement continues at72E7;
// zero stage or unexpired/unordered timer proceeds directly to70B0.
template<std::size_t Capacity>
bool stormbolt_run_cleanup_route(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,const StormboltCleanupBankApi& cleanup,
    const StormboltDecayEffectApi& effect,const StormboltDecayOwnerApi& owner_api,
    const StormboltMaterialColorApi& material,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa) {
    StormboltCleanupDecayResult result{};
    if(!stormbolt_cleanup_and_decay(store,token,initial_actor,cleanup,&result)) return false;
    if(result.decay_owner) {
        void* component{};
        std::memcpy(&component,static_cast<std::byte*>(result.decay_owner)+0x8808,sizeof(component));
        StormboltStageState state{};
        if(!stormbolt_cleanup_bank(component,0x1980,cleanup) || !store.snapshot(token,state)) return false;
        if(state.counter) {
            void* owner{};
            std::memcpy(&owner,reinterpret_cast<const std::byte*>(token.address)+0x9B0,sizeof(owner));
            if(!owner) return false;
            std::memcpy(&component,static_cast<std::byte*>(owner)+0x8808,sizeof(component));
            if(!stormbolt_update_decay_effect(component,state.counter,effect) ||
               !store.snapshot(token,state)) return false;
        }
        if(!stormbolt_cleanup_decay_latch(store,token,owner_api,cleanup)) return false;
    }
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
