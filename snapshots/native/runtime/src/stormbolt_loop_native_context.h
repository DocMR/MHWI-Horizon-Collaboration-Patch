#pragma once
#include "stormbolt_shell_hook.h"
#include "stormbolt_named_event_parameters.h"

namespace stormslinger {
struct StormboltLoopNativeContext {
    void* owner=nullptr;
    void* actor=nullptr;
    void* container=nullptr;
    float step=0;
    std::array<float,3> position{};
    std::array<std::array<float,3>,3> defaults{};
    bool actor_available=false;
};
// Borrowed component/owner lifetime must come from the synchronous native
// update boundary. Readable memory alone does not establish object lifetime.
inline bool read_stormbolt_loop_native_context(
    void* component,std::uintptr_t componentVtable,
    void* (*containerAt)(void*),const float* nativeFirst,const float* nativeSecond,
    StormboltLoopNativeContext& output) {
    const auto address=reinterpret_cast<std::uintptr_t>(component);
    std::uintptr_t vtable=0;
    StormboltLoopNativeContext candidate{};
    if (!componentVtable || !read_stormbolt_field(address,0,vtable) || vtable!=componentVtable ||
        !read_stormbolt_field(address,0x60,candidate.owner) || !candidate.owner ||
        !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(candidate.owner),0x68,candidate.step) ||
        !read_stormbolt_field(address,0xF0,candidate.actor)) return false;
    if (candidate.actor) {
        std::uint8_t flags=0;
        if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(candidate.actor),0xC,flags)) return false;
        candidate.actor_available=(flags&0xE)!=0;
    }
    if (candidate.actor_available) {
        void* sound=nullptr;
        if (!containerAt || !nativeFirst || !nativeSecond ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(candidate.owner),0xA18,sound) || !sound ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(candidate.actor),0x160,candidate.position) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(nativeFirst),0,candidate.defaults[0]) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(nativeSecond),0,candidate.defaults[1])) return false;
        candidate.defaults[2]=candidate.defaults[0];
        candidate.container=containerAt(sound);
        if (!candidate.container) return false;
    }
    output=candidate;
    return true;
}
} // namespace stormslinger
