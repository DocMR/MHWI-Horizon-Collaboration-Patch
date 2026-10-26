#pragma once
#include "owned_vtable_pointer.h"
#include <cstdint>
#include <cstring>

namespace stormslinger {
// B98F90 copies one record's label, input and style to its 24-byte stack
// parameters, then invokes Operations widget VT+40 before text assembly.
// Only the transient label index is changed; native input glyphs/styles remain.
struct StormboltOperationHintBindings {
    std::uintptr_t widget_vtable{};
    void (*original)(void*,void*){};
    bool (*source_mode)(bool& loaded){};
    std::uintptr_t window_vtable{};
    void (*original_update)(void*){};
};
inline std::uint32_t stormbolt_special_hint_label(std::uint32_t label,bool loaded) {
    // Existing localized buttonGuide labels: Place Wyvernblast -> Load/Unload
    // Special Ammo. Never change the global text pool or an unrelated label.
    return label==397 ? (loaded?406u:395u) : label;
}
template<bool Training=false>
class StormboltOperationHintHook {
    StormboltOperationHintBindings bindings_{};
    OwnedVtablePointer<> slot_;
    OwnedVtablePointer<> update_slot_;
    void* last_window_{}; // identity only; never dereferenced outside its callback
    unsigned last_mode_{};
    inline static StormboltOperationHintHook* active_{};
    static void callback(void* widget,void* parameters) {
        auto& self=*active_;
        self.bindings_.original(widget,parameters);
        bool loaded=false;
        if(parameters && self.bindings_.source_mode(loaded)) {
            auto& label=*static_cast<std::uint32_t*>(parameters);
            label=stormbolt_special_hint_label(label,loaded);
        }
    }
    static void update(void* window) {
        auto& self=*active_;
        bool loaded=false;
        const unsigned mode=self.bindings_.source_mode(loaded)?(loaded?2u:1u):0u;
        // Native record selection need not change when part8/mode changes.
        // Invalidate its existing layout, letting native animation/input gates
        // defer reconstruction. Never call the renderer outside that lifecycle.
        if((window==self.last_window_ && mode!=self.last_mode_) ||
            (window!=self.last_window_ && mode!=0)) {
            const unsigned char dirty=1;
            std::memcpy(static_cast<unsigned char*>(window)+(Training?0x38D4:0x6371),&dirty,1);
        }
        self.last_window_=window;self.last_mode_=mode;
        self.bindings_.original_update(window);
    }
public:
    bool install(const StormboltOperationHintBindings& bindings) {
        if(active_ || !bindings.widget_vtable || !bindings.original || !bindings.source_mode ||
            (bool(bindings.window_vtable)!=bool(bindings.original_update))) return false;
        bindings_=bindings;last_window_=nullptr;last_mode_=0;active_=this;
        if(!slot_.install(reinterpret_cast<void*>(bindings.widget_vtable+0x40),
            reinterpret_cast<void*>(bindings.original),reinterpret_cast<void*>(&callback))) {
            if(!slot_.active()) active_=nullptr;
            return false;
        }
        return !bindings.window_vtable || update_slot_.install(
            reinterpret_cast<void*>(bindings.window_vtable+0x260),
            reinterpret_cast<void*>(bindings.original_update),reinterpret_cast<void*>(&update));
    }
    bool restore() {
        if(!update_slot_.restore() || !slot_.restore()) return false;
        if(active_==this) active_=nullptr;
        return true;
    }
};
} // namespace stormslinger
