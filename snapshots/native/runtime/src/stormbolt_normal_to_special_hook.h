#pragma once
#include "owned_vtable_pointer.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
// Legacy type name; PS3A1F980 is actual DetonateBomb; production inherits PC1892C10.
using StormboltActionUpdate = bool (*)(void* action);
using StormboltActionEventObject = void* (*)(void* actor);
using StormboltActionChannelQuery = bool (*)(void* actor, std::uint32_t channel);
struct StormboltNormalToSpecialBindings {
    void* slot = nullptr; // image+33B17F0+38, never patch shared false stub.
    StormboltActionUpdate original = nullptr; // image+240720
    std::uintptr_t action_vtable = 0, actor_vtable = 0, weapon_vtable = 0;
    StormboltActionEventObject event_object = nullptr; // image+1F62070
    StormboltActionChannelQuery query = nullptr; // image+1BFD4E0 at actor vt+2C0
    bool (*dispatch_ready)() = nullptr; // Optional coordinator publication control.
};

inline bool stormbolt_action_event_writable(std::uintptr_t event) noexcept {
    constexpr auto maximum = (std::numeric_limits<std::uintptr_t>::max)();
    if (!event || event > maximum - 0x2C) return false;
    auto current = event + 0x28;
    const auto end = event + 0x2C;
    while (current < end) {
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(reinterpret_cast<void*>(current), &memory, sizeof(memory)) ||
            memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD)) return false;
        const auto access = memory.Protect & 0xFF;
        if (access != PAGE_READWRITE && access != PAGE_WRITECOPY &&
            access != PAGE_EXECUTE_READWRITE && access != PAGE_EXECUTE_WRITECOPY) return false;
        const auto base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (memory.RegionSize > maximum - base || base + memory.RegionSize <= current) return false;
        current = base + memory.RegionSize;
    }
    return true;
}

// Complete PS cNormalToSpecial update only; PS entry and exit are empty too.
// Process-lifetime component. Bootstrap must authenticate native code/types,
// install with the lifecycle hooks under quiescence, and establish live native
// action/actor/weapon/event ownership and serialized event access. Memory checks
// and sidecar tokens alone do not establish these native lifetime guarantees.
// The independent cSpecialToNormal (ammo40, effects, etc.) remains separate.
template<std::size_t Capacity>
class StormboltNormalToSpecialHook {
    StormboltInstanceStore<Capacity>& store_;
    StormboltNormalToSpecialBindings bindings_{};
    OwnedVtablePointer<> pointer_;
    inline static StormboltNormalToSpecialHook* active_ = nullptr;

    bool resolve(void* action, std::uintptr_t& actor, StormboltActionChannelQuery& query) {
        std::uintptr_t vtable = 0, weapon = 0, owner = 0;
        std::uint8_t flags = 0;
        struct Identity { std::uint32_t category, id; } identity{};
        const auto address = reinterpret_cast<std::uintptr_t>(action);
        if (!read_stormbolt_field(address, 0, vtable) || vtable != bindings_.action_vtable ||
            !read_stormbolt_field(address, 0x28, actor) ||
            !read_stormbolt_field(actor, 0, vtable) || vtable != bindings_.actor_vtable ||
            !read_stormbolt_field(actor, 0xC, flags) || !(flags & 0xE) ||
            !read_stormbolt_field(vtable, 0x2C0, query) || query != bindings_.query ||
            !read_stormbolt_field(actor, 0x76B0, weapon) ||
            !read_stormbolt_field(weapon, 0, vtable) || vtable != bindings_.weapon_vtable ||
            !read_stormbolt_field(weapon, 0xC, flags) || !(flags & 0xE) ||
            !read_stormbolt_field(weapon, 0x9B0, owner) || owner != actor ||
            !read_stormbolt_field(weapon, 0x9F8, identity) || identity.category != 13 ||
            (identity.id != 238 && identity.id != 239)) return false;
        StormboltInstanceToken token{};
        return store_.token_for_live_address(weapon, token); // No lazy initialization.
    }
    __declspec(noinline) static bool callback(void* action) {
        auto& hook_state = *active_;
        if (hook_state.bindings_.dispatch_ready && !hook_state.bindings_.dispatch_ready())
            return hook_state.bindings_.original(action);
        std::uintptr_t actor = 0;
        StormboltActionChannelQuery query = nullptr;
        bool selected = false;
        try { selected = hook_state.resolve(action, actor, query); }
        catch (...) { selected = false; }
        if (!selected) return hook_state.bindings_.original(action);

        // Native calls run outside the sidecar lock and exception handler.
        // Do not swallow native exceptions or call original again after a write.
        const auto event = reinterpret_cast<std::uintptr_t>(
            hook_state.bindings_.event_object(reinterpret_cast<void*>(actor)));
        std::uint32_t flags = 0;
        if (!read_stormbolt_field(event, 0x28, flags) ||
            ((flags & 1) && !stormbolt_action_event_writable(event)))
            return hook_state.bindings_.original(action);
        if (flags & 1) {
            flags &= ~std::uint32_t{1};
            std::memcpy(reinterpret_cast<void*>(event + 0x28), &flags, sizeof(flags));
        }
        return query(reinterpret_cast<void*>(actor), 0);
    }
public:
    explicit StormboltNormalToSpecialHook(StormboltInstanceStore<Capacity>& instances) : store_(instances) { }
    StormboltNormalToSpecialHook(const StormboltNormalToSpecialHook&) = delete;
    StormboltNormalToSpecialHook& operator=(const StormboltNormalToSpecialHook&) = delete;
    bool active() const noexcept { return pointer_.active(); }
    bool install(const StormboltNormalToSpecialBindings& bindings) {
        constexpr auto maximum = (std::numeric_limits<std::uintptr_t>::max)();
        if (active_ || !bindings.action_vtable || bindings.action_vtable > maximum - 0x38 ||
            reinterpret_cast<std::uintptr_t>(bindings.slot) != bindings.action_vtable + 0x38 ||
            !bindings.original || !bindings.actor_vtable || !bindings.weapon_vtable ||
            !bindings.event_object || !bindings.query) return false;
        bindings_ = bindings;
        active_ = this;
        return pointer_.install(bindings.slot, reinterpret_cast<void*>(bindings.original),
                                reinterpret_cast<void*>(&callback));
    }
    bool restore() { return pointer_.restore(); }
};
} // namespace stormslinger
