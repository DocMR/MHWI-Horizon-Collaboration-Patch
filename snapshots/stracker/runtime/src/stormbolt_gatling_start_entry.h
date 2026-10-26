#pragma once
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
// Legacy type name; PS3A20080 is actual SpecialToNormal (registered DTI6EE25F0).
struct StormboltGatlingEntryApi {
    StormboltSpecialToNormalApi animation{};
    // Audited PC endpoints: manager500ECA0, getter1B42010, table33D8340.
    // Bootstrap authenticates these and borrows manager/table lifetime.
    void** player_manager_slot = nullptr;
    void* (*local_player)(void* manager) = nullptr;
    const std::uint32_t* ammo_items = nullptr; // Exactly41 native entries.

};

// PS3A20080 entry dataflow using the already-audited native motion dispatcher
// to preserve classification/flag gates and reloads after each request.
// Native addresses are audited; executable guard/bootstrap installation is pending.
template<std::size_t Capacity>
class StormboltGatlingStartEntry {
    StormboltInstanceStore<Capacity>& store_;
    const StormboltGatlingEntryApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{}; std::memcpy(&value,static_cast<const std::byte*>(object)+offset,sizeof(value)); return value;
    }
    template<class T> static void write(void* object, std::size_t offset, T value) {
        std::memcpy(static_cast<std::byte*>(object)+offset,&value,sizeof(value));
    }
    static bool live(void* object) { return object && (read<std::uint8_t>(object,0xC)&0xE); }
public:
    StormboltGatlingStartEntry(StormboltInstanceStore<Capacity>& store, const StormboltGatlingEntryApi& api)
        : store_(store), api_(api) { }
    StormboltSpecialActionStatus enter(void* action) {
        if (!action || !api_.animation.motion || !api_.player_manager_slot || !api_.local_player || !api_.ammo_items)
            return StormboltSpecialActionStatus::unavailable;
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return StormboltSpecialActionStatus::unavailable;
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13)
            return StormboltSpecialActionStatus::unavailable;
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token))
            return StormboltSpecialActionStatus::unavailable;
        write(weapon,0x2350,std::uint8_t{0});
        auto* owner=read<void*>(weapon,0x9B0);
        std::uint32_t ammo=0;
        StormboltStageState ignored{};
        if (live(owner)) {
            auto* local=api_.local_player(*api_.player_manager_slot);
            if (!store_.snapshot(token,ignored)) return StormboltSpecialActionStatus::invalidated;
            if (owner==local) {
                const auto item=read<std::uint32_t>(weapon,0x27DC);
                if (!item) ammo=0xFF;
                else for (std::uint32_t index=0; index<41; ++index) {
                    if (api_.ammo_items[index]==item) { ammo=index; break; }
                }
            }
        }
        write(weapon,0x27C8,ammo);
        for (std::uint32_t layer=0; layer<3; ++layer) {
            api_.animation.motion(actor,0x200u+layer,0xC0FFu-layer,
                layer==0 ? 2u : layer==1 ? 1u : 3u,0,-1.0f,2.0f);
            if (layer<2) {
                actor=read<void*>(action,0x28);
                if (!live(actor)) return StormboltSpecialActionStatus::invalidated;
            }
        }
        return StormboltSpecialActionStatus::processed;
    }
};
} // namespace stormslinger
