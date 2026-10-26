#pragma once
#include "stormbolt_instance_store.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
struct StormboltDamageExceptionState {bool absorbed_positive_input=false;};
struct StormboltDamageExceptionToken {
    std::uintptr_t address=0;
    std::uint64_t generation=0;
    explicit operator bool() const noexcept {return address && generation;}
};
template<std::size_t Capacity>
using StormboltDamageExceptionStore=StormboltInstanceStore<Capacity,
    StormboltDamageExceptionState,StormboltDamageExceptionToken>;

// Capture before native118EFE0, then mark after it returns using the saved
// original XMM1 input. Lifecycle hooks already track cpHmDamageCheck.
template<std::size_t Capacity>
bool capture_stormbolt_damage_exception_for_absorption(void* stateObject,
    std::uintptr_t damageVtable,StormboltDamageExceptionStore<Capacity>& store,
    StormboltDamageExceptionToken& output) {
    if (!stateObject || !damageVtable) return false;
    std::uintptr_t actor=0,component=0,vtable=0;
    if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(stateObject),0xDB0,actor) ||
        !actor || !read_stormbolt_field(actor,0x7658,component) || !component ||
        !read_stormbolt_field(component,0,vtable) || vtable!=damageVtable) return false;
    StormboltDamageExceptionToken token{};
    if (!store.token_for_live_address(component,token)) return false;
    output=token;return true;
}
template<std::size_t Capacity>
bool mark_stormbolt_damage_exception_for_captured_absorption(
    StormboltDamageExceptionToken token,float originalInput,
    StormboltDamageExceptionStore<Capacity>& store) {
    return store.transact(token,[&](auto& state) noexcept {
        if (originalInput>0.0f) state.absorbed_positive_input=true;
        return true;
    });
}
// Convenience for an already completed synchronous fixture. Native bridges
// must capture before invoking original and mark the same token afterwards.
template<std::size_t Capacity>
bool mark_stormbolt_damage_exception_after_absorption(void* stateObject,float originalInput,
    std::uintptr_t damageVtable,StormboltDamageExceptionStore<Capacity>& store) {
    StormboltDamageExceptionToken token{};
    return capture_stormbolt_damage_exception_for_absorption(stateObject,damageVtable,store,token) &&
        mark_stormbolt_damage_exception_for_captured_absorption(token,originalInput,store);
}
// Native update must finish before clearing; do not clear on event reads.
template<std::size_t Capacity>
bool clear_stormbolt_damage_exception_after_update(StormboltDamageExceptionToken token,
    StormboltDamageExceptionStore<Capacity>& store) {
    return store.transact(token,[](auto& state) noexcept {
        state.absorbed_positive_input=false;return true;
    });
}
} // namespace stormslinger
