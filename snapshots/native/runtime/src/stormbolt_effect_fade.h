#pragma once
#include "stormbolt_effect_color.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
// PS4997E4A..7F67. This is a selected continuation, not an unconditional tick.
// Caller establishes rising (PS288C), current base color and native lifetimes.
template<std::size_t Capacity>
bool stormbolt_update_effect_fade(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,bool rising,std::uint32_t base_color,
    const StormboltEffectColorApi& api) {
    auto* weapon=reinterpret_cast<std::byte*>(token.address);
    bool enabled=false;
    float alpha{};
    if (!store.transact(token,[&](auto& state) noexcept {
        if (!state.effect_latch) return true;
        float step{};std::memcpy(&step,weapon+0x68,4);
        enabled=advance_stormbolt_effect_alpha(state,step,rising);
        alpha=state.effect_alpha;
        return true;
    })) return false;
    if (!enabled) return true;
    // Native effect access happens after the alpha commit and outside the lock.
    void* owner{};std::memcpy(&owner,weapon+0x9B0,sizeof(owner));
    if (!owner) return false;
    void* component{};std::memcpy(&component,static_cast<std::byte*>(owner)+0x8808,sizeof(component));
    if (!stormbolt_write_effect_color(component,alpha,base_color,api)) return false;
    StormboltStageState after{};
    return store.snapshot(token,after); // Native callback invalidation: no replay.
}
} // namespace stormslinger
