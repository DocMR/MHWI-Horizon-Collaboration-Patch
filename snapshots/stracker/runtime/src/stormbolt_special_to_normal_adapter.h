#pragma once
#include "stormbolt_transition_ammo_adapter.h"
#include "stormbolt_player_effect_adapter.h"

namespace stormslinger {
// Legacy type name; PS3A1FC60 is actual NormalToSpecial (registered DTI6EE25B8).
struct StormboltSpecialToNormalApi {
    bool (*motion)(void* actor, std::uint32_t packed, std::uint32_t id,
        std::uint32_t mode, std::uint32_t extra, float start, float rate) = nullptr; // 1F72AE0
    void (*actor_flag)(void* actor, std::uint32_t flag) = nullptr; // 1F604B0
    void* (*event_object)(void* actor) = nullptr; // 1F62070
    void (*weapon_effect)(void* component, std::uint32_t group,
        std::uint32_t record, std::uint32_t parameter) = nullptr; // AE2910, return ignored
    bool (*query)(void* actor, std::uint32_t channel) = nullptr; // virtual2C0 /1BFD4E0
    bool complete() const noexcept {
        return motion && actor_flag && event_object && weapon_effect && query;
    }
};
enum class StormboltSpecialActionStatus { processed, unavailable, invalidated };
struct StormboltSpecialActionResult {
    StormboltSpecialActionStatus status = StormboltSpecialActionStatus::unavailable;
    bool animation_complete = false;
};

// Complete PS cSpecialToNormal entry/update/exit dataflow as a candidate core.
// Not installed in a game vtable. Caller authenticates native APIs/types, binds
// effect/save APIs, establishes native borrowed lifetimes and serialized access,
// and verifies motion/EFX resources before activation. Store tokens protect only
// sidecar identity. Native calls run without locks/catch. Once entered, callers
// must not replay an event or invoke an original action on an invalidated result.
template<std::size_t Capacity>
class StormboltSpecialToNormalAdapter {
    StormboltInstanceStore<Capacity>& store_;
    StormboltTransitionAmmoAdapter<Capacity>& ammo_;
    const StormboltPlayerEffectAdapter& effects_;
    const StormboltSpecialToNormalApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{};
        std::memcpy(&value, static_cast<const std::byte*>(object)+offset, sizeof(value));
        return value;
    }
    template<class T> static void write(void* object, std::size_t offset, T value) {
        std::memcpy(static_cast<std::byte*>(object)+offset, &value, sizeof(value));
    }
    static bool live(const void* object) {
        return object && (read<std::uint8_t>(object, 0xC) & 0xE);
    }
    bool weapon_for(void* actor, void*& weapon, StormboltInstanceToken& token) {
        if (!live(actor)) return false;
        weapon = read<void*>(actor, 0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon, 0x9F8) != 13) return false;
        const auto id = read<std::uint32_t>(weapon, 0x9FC);
        return (id == 238 || id == 239) &&
            store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon), token);
    }
    bool current(void* action, void*& actor, void*& weapon, StormboltInstanceToken& token) {
        actor = read<void*>(action, 0x28);
        return weapon_for(actor, weapon, token);
    }
    static void model_bit(void* weapon, bool set) {
        auto flags = read<std::uint32_t>(weapon, 0x2D0);
        flags = set ? (flags | 0x400u) : (flags & ~0x400u);
        write(weapon, 0x2D0, flags);
    }
public:
    StormboltSpecialToNormalAdapter(StormboltInstanceStore<Capacity>& store,
        StormboltTransitionAmmoAdapter<Capacity>& ammo,
        const StormboltPlayerEffectAdapter& effects, const StormboltSpecialToNormalApi& api)
        : store_(store), ammo_(ammo), effects_(effects), api_(api) { }

    StormboltSpecialActionStatus enter(void* action) {
        void* actor = nullptr;
        void* weapon = nullptr;
        StormboltInstanceToken token{};
        if (!api_.complete() || !current(action, actor, weapon, token))
            return StormboltSpecialActionStatus::unavailable;
        // Dispatcher implements native class/actor gates for each request.
        // PS reads action+28 again after each native call, even a rejected one.
        for (std::uint32_t layer = 0; layer < 3; ++layer) {
            const std::uint32_t mode = layer == 0 ? 2u : (layer == 1 ? 1u : 3u);
            api_.motion(actor, 0x200u+layer, 0xC0FCu-layer, mode, 0, -1.0f, 2.0f);
            if (!current(action, actor, weapon, token)) return StormboltSpecialActionStatus::invalidated;
        }
        auto* owner = read<void*>(weapon, 0x9B0);
        if (!live(owner)) return StormboltSpecialActionStatus::invalidated;
        api_.weapon_effect(read<void*>(owner, 0x8808), 0x7D0, 0, 0xFFFFFFFF);
        return StormboltSpecialActionStatus::processed;
    }

    StormboltSpecialActionResult update(void* action) {
        void* actor = nullptr;
        void* weapon = nullptr;
        StormboltInstanceToken token{};
        if (!api_.complete() || !current(action, actor, weapon, token)) return {};
        api_.actor_flag(actor, 0x5D);
        api_.actor_flag(actor, 0x28);
        auto* event = api_.event_object(actor);
        auto trigger = read<std::uint32_t>(event, 0x190);
        if (trigger & 2) {
            write(event, 0x190, trigger & ~2u);
            // This branch uses the actor held before the native save callbacks.
            if (!weapon_for(actor, weapon, token) ||
                ammo_.apply(weapon, token) != StormboltTransitionAmmoResult::completed)
                return {StormboltSpecialActionStatus::invalidated, false};
        }
        // PS retains the original event pointer but reloads action actor/weapon.
        if (!current(action, actor, weapon, token))
            return {StormboltSpecialActionStatus::invalidated, false};
        const auto flags = read<std::uint32_t>(event, 0x178); // secondary.mFlag2
        model_bit(weapon, !(flags & 0x08000000u));
        if (!(flags & 0x10000000u)) {
            if (!effects_.update_transition_effect(read<void*>(actor, 0x8808), false))
                return {StormboltSpecialActionStatus::invalidated, false};
        } else {
            auto* owner = read<void*>(weapon, 0x9B0);
            // PS enable branch checks weapon owner; disable uses action actor.
            if (live(owner) && !effects_.update_transition_effect(read<void*>(owner, 0x8808), true))
                return {StormboltSpecialActionStatus::invalidated, false};
        }
        actor = read<void*>(action, 0x28);
        if (!live(actor)) return {StormboltSpecialActionStatus::invalidated, false};
        return {StormboltSpecialActionStatus::processed, api_.query(actor, 0)};
    }

    StormboltSpecialActionStatus exit(void* action) {
        void* actor = nullptr;
        void* weapon = nullptr;
        StormboltInstanceToken token{};
        if (!current(action, actor, weapon, token)) return StormboltSpecialActionStatus::unavailable;
        model_bit(weapon, true);
        return StormboltSpecialActionStatus::processed;
    }
};
} // namespace stormslinger
