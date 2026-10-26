#pragma once
#include "stormbolt_normal_to_special_hook.h"
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
enum class StormboltSpecialActionPhase { enter, update, exit };
using StormboltActionBoundary = void (*)(void* action);
struct StormboltSpecialToNormalBindings {
    std::uintptr_t action_vtable = 0; // image+33B1858, slots30/38/40 only.
    std::uintptr_t actor_vtable = 0, weapon_vtable = 0;
    StormboltActionBoundary original_enter = nullptr, original_exit = nullptr;
    StormboltActionUpdate original_update = nullptr;
    // Required read-only preflight: authenticate all dependent native APIs,
    // objects and resources and establish their borrowed lifetime/serialized
    // access for the whole callback, including native callback re-reads.
    // This is not supplied by a readable-memory or sidecar-token check.
    bool (*ready)(void* action, StormboltSpecialActionPhase phase) = nullptr;
};

// Candidate scoped dispatcher. Native bootstrap/preflight is still required;
// supplying an unconditional ready callback is not sufficient for game use.
// Install/restore require external quiescence. Keep this object, core and its
// dependencies alive until every pointer and protection has been restored.
template<std::size_t Capacity,
    class Core = StormboltSpecialToNormalAdapter<Capacity>,
    class Pointer = OwnedVtablePointer<>, bool IncludeExit = true>
class StormboltSpecialToNormalHook {
    StormboltInstanceStore<Capacity>& store_;
    Core& core_;
    StormboltSpecialToNormalBindings bindings_{};
    Pointer enter_, update_, exit_;
    inline static StormboltSpecialToNormalHook* owner_ = nullptr;

    bool selected(void* action, StormboltSpecialActionPhase phase) {
        const auto address = reinterpret_cast<std::uintptr_t>(action);
        std::uintptr_t table = 0, actor = 0, weapon = 0, owner = 0;
        std::uint8_t flags = 0;
        struct Identity { std::uint32_t category, id; } identity{};
        if (!read_stormbolt_field(address, 0, table) || table != bindings_.action_vtable ||
            !read_stormbolt_field(address, 0x28, actor) ||
            !read_stormbolt_field(actor, 0, table) || table != bindings_.actor_vtable ||
            !read_stormbolt_field(actor, 0xC, flags) || !(flags & 0xE) ||
            !read_stormbolt_field(actor, 0x76B0, weapon) ||
            !read_stormbolt_field(weapon, 0, table) || table != bindings_.weapon_vtable ||
            !read_stormbolt_field(weapon, 0xC, flags) || !(flags & 0xE) ||
            !read_stormbolt_field(weapon, 0x9B0, owner) || owner != actor ||
            !read_stormbolt_field(weapon, 0x9F8, identity) || identity.category != 13 ||
            (identity.id != 238 && identity.id != 239)) return false;
        StormboltInstanceToken token{};
        return store_.token_for_live_address(weapon, token) && bindings_.ready(action, phase);
    }
    static void on_enter(void* action) {
        auto& hook = *owner_;
        if (!hook.selected(action, StormboltSpecialActionPhase::enter)) {
            hook.bindings_.original_enter(action); return;
        }
        hook.core_.enter(action); // Never replay original after entering core.
    }
    static bool on_update(void* action) {
        auto& hook = *owner_;
        if (!hook.selected(action, StormboltSpecialActionPhase::update))
            return hook.bindings_.original_update(action);
        const auto result = hook.core_.update(action);
        return result.status == StormboltSpecialActionStatus::processed && result.animation_complete;
    }
    static void on_exit(void* action) {
        auto& hook = *owner_;
        if (!hook.selected(action, StormboltSpecialActionPhase::exit)) {
            hook.bindings_.original_exit(action); return;
        }
        hook.core_.exit(action);
    }
public:
    StormboltSpecialToNormalHook(StormboltInstanceStore<Capacity>& store, Core& core)
        : store_(store), core_(core) { }
    StormboltSpecialToNormalHook(const StormboltSpecialToNormalHook&) = delete;
    StormboltSpecialToNormalHook& operator=(const StormboltSpecialToNormalHook&) = delete;
    bool active() const noexcept { return enter_.active() || update_.active() || exit_.active(); }
    bool install(const StormboltSpecialToNormalBindings& bindings) {
        constexpr auto maximum = (std::numeric_limits<std::uintptr_t>::max)();
        if (owner_ || !bindings.action_vtable || bindings.action_vtable > maximum - 0x48 ||
            (bindings.action_vtable & 7) || !bindings.actor_vtable || !bindings.weapon_vtable ||
            !bindings.original_enter || !bindings.original_update || (IncludeExit && !bindings.original_exit) || !bindings.ready)
            return false;
        const std::uintptr_t originals[]{reinterpret_cast<std::uintptr_t>(bindings.original_enter),
            reinterpret_cast<std::uintptr_t>(bindings.original_update),
            reinterpret_cast<std::uintptr_t>(bindings.original_exit)};
        for (std::size_t i = 0; i < (IncludeExit ? 3u : 2u); ++i) {
            std::uintptr_t value = 0;
            if (!read_stormbolt_field(bindings.action_vtable, 0x30+i*8, value) || value != originals[i])
                return false;
        }
        bindings_ = bindings;
        owner_ = this;
        // On partial failure the caller must restore; no dependency is released.
        if (!(enter_.install(reinterpret_cast<void*>(bindings.action_vtable+0x30),
                   reinterpret_cast<void*>(bindings.original_enter), reinterpret_cast<void*>(&on_enter)) &&
            update_.install(reinterpret_cast<void*>(bindings.action_vtable+0x38),
                   reinterpret_cast<void*>(bindings.original_update), reinterpret_cast<void*>(&on_update)))) return false;
        if constexpr (IncludeExit) return exit_.install(reinterpret_cast<void*>(bindings.action_vtable+0x40),
                   reinterpret_cast<void*>(bindings.original_exit), reinterpret_cast<void*>(&on_exit));
        else return true;
    }
    bool restore() {
        // Stop immediately on failure; retain earlier callbacks and dependencies.
        if (!exit_.restore() || !update_.restore() || !enter_.restore()) return false;
        if (owner_ == this) owner_ = nullptr;
        return true;
    }
};
} // namespace stormslinger
