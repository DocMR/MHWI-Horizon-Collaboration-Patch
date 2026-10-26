#pragma once
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltCleanupDecayResult {
    void* decay_owner=nullptr; // Non-null only when this invocation decremented.
};
// PS4997179..72E7 branch only. Caller has already selected this branch using the
// complete outer gates, owns native lifetimes and serializes weapon updates.
template<std::size_t Capacity>
bool stormbolt_cleanup_and_decay(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* actor,const StormboltCleanupBankApi& api,
    StormboltCleanupDecayResult* result=nullptr) {
    if(result) *result={};
    auto* weapon=reinterpret_cast<std::byte*>(token.address);
    StormboltStageState current{};
    if (!store.snapshot(token,current) || !stormbolt_cleanup_stage_effects(weapon,actor,api)) return false;
    void* decay_owner=nullptr;
    const bool success=store.transact(token,[&](auto& state) noexcept {
        state.effect_latch=0;
        if (!state.counter) return true;
        float step{}; std::memcpy(&step,weapon+0x68,4);
        state.decay_timer-=step;
        // Native 0 vs remaining / JB skips positive and unordered results.
        if (!(state.decay_timer<=0.0f)) return true;
        const auto next=(std::min)(state.counter-1,std::uint32_t{8});
        void* owner{}; std::memcpy(&owner,weapon+0x9B0,sizeof(owner));
        if (!owner) return false;
        void* parameter{}; std::memcpy(&parameter,static_cast<std::byte*>(owner)+0x7810,sizeof(parameter));
        if (!parameter) return false;
        state.counter=next;
        std::memcpy(&state.decay_timer,static_cast<std::byte*>(parameter)+0x160C+40*next,4);
        decay_owner=owner;
        return true;
    });
    if(success && result) result->decay_owner=decay_owner;
    return success;
}
} // namespace stormslinger
