#pragma once
#include "stormbolt_instance_store.h"

namespace stormslinger {
// PS4997E23..7E43: local continuation only, including no-decrement frames.
// Run after any conditional 1AA0 cleanup. Remote continuation does not do this.
// Caller proves stage0..8 and native lifetimes; no arbitrary row read/clamping.
template<std::size_t Capacity>
bool stormbolt_reload_local_decay_action_timer(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token) {
    return store.transact(token,[&](auto& state) noexcept {
        if (state.counter>8) return false;
        auto* weapon=reinterpret_cast<const std::byte*>(token.address);
        void* owner{};std::memcpy(&owner,weapon+0x9B0,sizeof(owner));
        if (!owner) return false;
        void* parameter{};
        std::memcpy(&parameter,static_cast<const std::byte*>(owner)+0x7810,sizeof(parameter));
        if (!parameter) return false;
        std::memcpy(&state.action_timer,static_cast<const std::byte*>(parameter)+0x1608+40*state.counter,4);
        return true;
    });
}
} // namespace stormslinger
