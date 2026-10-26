#pragma once
#include "stormbolt_decay_effect.h"
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_effect_color.h"

namespace stormslinger {
struct StormboltShotEffectApi {
    StormboltDecayEffectApi effect{};
    StormboltCleanupBankApi cleanup{};
    StormboltDecayOwnerApi owner{};
    StormboltEffectColorApi color{};
    std::uint32_t (*attachment)(void*){};
    std::uint32_t base_color=ps_stormbolt_color_default;
};
// PS49985E2..8A64, after self damage and energy subtraction, before shell
// creation. This is a shot callback, not a new per-frame effect producer.
template<std::size_t Capacity>
bool stormbolt_shot_effects(StormboltInstanceStore<Capacity>& store,void* weapon,
    std::uint32_t shot,const StormboltShotEffectApi& api) {
    if(shot>8 || !api.effect.active || !api.effect.request || !api.effect.assign ||
       !api.attachment || !api.color.set_color) return false;
    StormboltInstanceToken token{};StormboltStageState state{};
    if(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return false;
    const auto component=[&]() -> void* {
        void* actor{};void* value{};
        std::memcpy(&actor,static_cast<std::byte*>(weapon)+0x9B0,sizeof(actor));
        if(actor) std::memcpy(&value,static_cast<std::byte*>(actor)+0x8808,sizeof(value));
        return value;
    };
    // Original table5A2B090 second DWORD, distinct from the shell selector.
    constexpr std::uint32_t fire_records[]={0,0,0,1,1,2,2,2,3};
    auto* current=component();
    if(!current) return false;
    api.effect.request(current,0x7DA,fire_records[shot],api.attachment(weapon));
    if(!store.snapshot(token,state)) return false;
    current=component();
    if(!stormbolt_cleanup_bank(current,0x1980,api.cleanup)) return false;
    current=component();
    if(!stormbolt_cleanup_bank(current,0x19E0,api.cleanup)) return false;
    if(shot==8) {
        current=component();if(!current) return false;
        // Final-stage discharge is record3; it is not the Last motion's7DD.
        api.effect.request(current,0x7DB,3,0xFFFFFFFF);
        if(!store.snapshot(token,state)) return false;
        if(!stormbolt_cleanup_bank(component(),0x1AA0,api.cleanup)) return false;
        return store.transact(token,[](auto& next) noexcept {next.effect_latch=0;return true;});
    }
    if(!store.snapshot(token,state)) return false;
    if(!stormbolt_update_decay_effect(component(),state.counter,api.effect)) return false;
    void* associated{};std::memcpy(&associated,static_cast<std::byte*>(weapon)+0x998,sizeof(associated));
    if(!associated) return true;
    std::uint8_t flags{};std::memcpy(&flags,static_cast<std::byte*>(associated)+0xC,1);
    if(!(flags&0xE)) return true;
    if(!api.owner.manager_slot || !*api.owner.manager_slot || !api.owner.local_player) return false;
    if(api.owner.local_player(*api.owner.manager_slot)!=associated) return true;
    if(!store.snapshot(token,state)) return false;
    if(state.counter<6) return true;
    current=component();if(!current) return false;
    std::byte* buffer{};std::memcpy(&buffer,static_cast<std::byte*>(current)+8,sizeof(buffer));
    if(!buffer) return false;
    if(!api.effect.active(buffer+0x1AA0)) {
        auto* result=api.effect.request(current,0x834,0,0xFFFFFFFF);
        std::memcpy(&buffer,static_cast<std::byte*>(current)+8,sizeof(buffer));
        if(!buffer) return false;
        api.effect.assign(buffer+0x1AA0,result);
        void* first{};std::memcpy(&first,buffer+0x1AA0,sizeof(first));
        if(!first) return false;
        std::memcpy(&flags,static_cast<std::byte*>(first)+0xC,1);
        if(!(flags&0xE)) return false;
        // PC22F9B70 writes41C and420/424, matching PS424 and428/42C.
        api.color.set_color(first,&api.base_color);
    }
    return store.transact(token,[](auto& next) noexcept {activate_stormbolt_effect_latch(next);return true;});
}
} // namespace stormslinger
