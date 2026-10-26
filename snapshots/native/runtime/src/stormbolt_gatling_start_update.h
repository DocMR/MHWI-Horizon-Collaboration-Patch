#pragma once
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
// Entire PS cShootLGatling update3A20270, not its separate entry3A20080.
// Caller authenticates action/actor/weapon types, API and resource readiness,
// and borrows native lifetimes with serialized access for the whole callback.
// Never replay the original action after an event has been consumed.
template<std::size_t Capacity>
class StormboltGatlingStartUpdate {
    StormboltInstanceStore<Capacity>& store_;
    const StormboltSpecialToNormalApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{};
        std::memcpy(&value, static_cast<const std::byte*>(object)+offset, sizeof(value));
        return value;
    }
    static bool live(const void* object) {
        return object && (read<std::uint8_t>(object, 0xC) & 0xE);
    }
    bool selected(void* actor, void*& weapon) {
        if (!live(actor)) return false;
        weapon = read<void*>(actor, 0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon, 0x9F8) != 13) return false;
        const auto id = read<std::uint32_t>(weapon, 0x9FC);
        StormboltInstanceToken token{};
        return (id == 238 || id == 239) &&
            store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon), token);
    }
public:
    StormboltGatlingStartUpdate(StormboltInstanceStore<Capacity>& store,
        const StormboltSpecialToNormalApi& api) : store_(store), api_(api) { }

    StormboltSpecialActionResult update(void* action) {
        if (!action || !api_.actor_flag || !api_.event_object || !api_.weapon_effect || !api_.query)
            return {};
        auto* actor = read<void*>(action, 0x28);
        void* weapon = nullptr;
        if (!selected(actor, weapon)) return {};
        api_.actor_flag(actor, 0x5D);
        api_.actor_flag(actor, 0x28);
        auto* event = api_.event_object(actor);
        if (!event) return {StormboltSpecialActionStatus::invalidated, false};
        auto bits = read<std::uint32_t>(event, 0x190);
        if (bits & 2) {
            bits &= ~std::uint32_t{2};
            std::memcpy(static_cast<std::byte*>(event)+0x190, &bits, sizeof(bits));
            // PS retains actor through flag calls, reads its weapon after the
            // event, then reloads action+28 only after the effect request.
            if (!selected(actor, weapon)) return {StormboltSpecialActionStatus::invalidated, false};
            auto* owner = read<void*>(weapon, 0x9B0);
            if (!live(owner)) return {StormboltSpecialActionStatus::invalidated, false};
            api_.weapon_effect(read<void*>(owner, 0x8808), 0x7D0, 0, 0xFFFFFFFF);
            actor = read<void*>(action, 0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated, false};
        }
        return {StormboltSpecialActionStatus::processed, api_.query(actor, 0)};
    }
};
} // namespace stormslinger
