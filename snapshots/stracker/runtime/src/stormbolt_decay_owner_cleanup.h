#pragma once
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltDecayOwnerApi {
    void** manager_slot=nullptr;
    void* (*local_player)(void*)=nullptr;
};
// Both after-decrement continuations only. PC998 is PS9F0 (the associated
// actor), distinct from PC9B0 used to find the effect component after the gate.
// Does not reset the local action timer or run the following fade continuation.
template<std::size_t Capacity>
bool stormbolt_cleanup_decay_latch(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,const StormboltDecayOwnerApi& owner_api,
    const StormboltCleanupBankApi& cleanup) {
    StormboltStageState state{};
    if (!store.snapshot(token,state)) return false;
    auto* weapon=reinterpret_cast<std::byte*>(token.address);
    void* associated{};std::memcpy(&associated,weapon+0x998,sizeof(associated));
    if (!associated) return true;
    std::uint8_t flags{};std::memcpy(&flags,static_cast<std::byte*>(associated)+0xC,1);
    if (!(flags&0xE)) return true;
    if (!owner_api.manager_slot || !*owner_api.manager_slot || !owner_api.local_player) return false;
    if (owner_api.local_player(*owner_api.manager_slot)!=associated) return true;
    if (!store.snapshot(token,state)) return false;
    if (state.counter>5) return true;
    void* owner{};std::memcpy(&owner,weapon+0x9B0,sizeof(owner));
    if (!owner) return false;
    void* component{};std::memcpy(&component,static_cast<std::byte*>(owner)+0x8808,sizeof(component));
    if (!stormbolt_cleanup_bank(component,0x1AA0,cleanup)) return false;
    return store.transact(token,[](auto& next) noexcept {next.effect_latch=0;return true;});
}
} // namespace stormslinger
