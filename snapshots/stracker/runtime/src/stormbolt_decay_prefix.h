#pragma once
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_decay_effect.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
enum class StormboltDecayPrefixResult { invalidated, no_decrement, continue_after_effect };
// Common portion of PS499768A / PS499785D through the 1980 recreation.
// no_decrement: remote ->70B0, local ->7E23. continue_after_effect: remote
// ->7C52, local ->7B74. These remaining continuations are NOT executed here.
// Caller establishes branch gates, serialized updates and native lifetimes.
template<std::size_t Capacity>
StormboltDecayPrefixResult stormbolt_decay_prefix(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_component,const StormboltCleanupBankApi& cleanup,
    const StormboltDecayEffectApi& effect) {
    constexpr auto invalid=StormboltDecayPrefixResult::invalidated;
    StormboltStageState state{};
    if (!store.snapshot(token,state) || !stormbolt_cleanup_bank(initial_component,0x19E0,cleanup)) return invalid;
    auto* weapon=reinterpret_cast<std::byte*>(token.address);
    void* decay_owner=nullptr;
    bool decremented=false;
    if (!store.transact(token,[&](auto& next) noexcept {
        if (!next.counter) return true;
        float step{};std::memcpy(&step,weapon+0x68,4);
        next.decay_timer-=step;
        if (!(next.decay_timer<=0.0f)) return true;
        std::memcpy(&decay_owner,weapon+0x9B0,sizeof(decay_owner));
        if (!decay_owner) return false;
        void* parameter{};
        std::memcpy(&parameter,static_cast<std::byte*>(decay_owner)+0x7810,sizeof(parameter));
        if (!parameter) return false;
        next.counter=(std::min)(next.counter-1,std::uint32_t{8});
        std::memcpy(&next.decay_timer,static_cast<std::byte*>(parameter)+0x160C+40*next.counter,4);
        decremented=true;
        return true;
    })) return invalid;
    if (!decremented) return StormboltDecayPrefixResult::no_decrement;
    void* component{};
    std::memcpy(&component,static_cast<std::byte*>(decay_owner)+0x8808,sizeof(component));
    if (!stormbolt_cleanup_bank(component,0x1980,cleanup) || !store.snapshot(token,state)) return invalid;
    if (state.counter) {
        void* owner{};std::memcpy(&owner,weapon+0x9B0,sizeof(owner));
        if (!owner) return invalid;
        std::memcpy(&component,static_cast<std::byte*>(owner)+0x8808,sizeof(component));
        if (!stormbolt_update_decay_effect(component,state.counter,effect)) return invalid;
        if (!store.snapshot(token,state)) return invalid;
    }
    // Preserve effect_latch, action_timer and shot_snapshot throughout. A failure
    // after native side effects cannot roll them back and must not be replayed.
    return StormboltDecayPrefixResult::continue_after_effect;
}
} // namespace stormslinger
