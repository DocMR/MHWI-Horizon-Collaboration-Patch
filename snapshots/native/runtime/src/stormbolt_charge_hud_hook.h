#pragma once
#include "stormbolt_charge_hud.h"
#include "owned_vtable_pointer.h"
#include <cstring>

namespace stormslinger {
struct StormboltChargeHudBindings {
    void* slot{};
    void (*original)(void*){};
    std::uintptr_t controller_vtable{};
    // Called after the inherited refresh, while the controller/owner are borrowed
    // by the native UI caller. Returns false for non-target/context/part states.
    bool (*input)(void*, StormboltChargeHudInput&){};
    StormboltChargeHudApi api{};
};

// PC B6B1A0 owns every node below. No retained GUI pointers or extra fields are
// installed in the native object. Ordinary refresh runs once even on removal.
class StormboltChargeHudHook {
    StormboltChargeHudBindings bindings_{};
    OwnedVtablePointer<> slot_;
    inline static StormboltChargeHudHook* active_{};
    template<class T> static T field(void* object,std::size_t offset) {
        T value{};
        std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(value));
        return value;
    }
    static StormboltChargeHudNodes nodes(void* controller) {
        StormboltChargeHudNodes result{};
        result.ordinary_root=field<void*>(controller,0xC8);
        result.charge_root=field<void*>(controller,0xF0);
        result.charge_child=field<void*>(controller,0xF8);
        result.on_off=field<void*>(controller,0x100);
        for(std::size_t i=0;i<result.bars.size();++i)
            result.bars[i]=field<void*>(controller,0x108+i*8);
        for(std::size_t i=0;i<result.selected_ammo.size();++i)
            result.selected_ammo[i]=field<void*>(controller,0x150+i*8);
        for(std::size_t i=0;i<result.segments.size();++i)
            result.segments[i]=field<void*>(controller,0x168+i*8);
        result.animation_context=field<void*>(controller,0x70);
        return result;
    }
    static bool complete(const StormboltChargeHudNodes& n) {
        if(!n.ordinary_root || !n.charge_root || !n.charge_child || !n.on_off) return false;
        for(auto* p:n.bars) if(!p) return false;
        for(auto* p:n.selected_ammo) if(!p) return false;
        for(auto* p:n.segments) if(!p) return false;
        return true;
    }
    static void callback(void* controller) {
        auto& self=*active_;
        self.bindings_.original(controller);
        StormboltChargeHudInput input{};
        if(field<std::uintptr_t>(controller,0)!=self.bindings_.controller_vtable ||
            !self.bindings_.input(controller,input)) return;
        const auto bound=nodes(controller);
        if(complete(bound)) refresh_stormbolt_charge_hud(bound,input,self.bindings_.api);
    }
public:
    bool install(const StormboltChargeHudBindings& bindings) {
        if(active_ || !bindings.original || !bindings.input || !bindings.controller_vtable ||
            reinterpret_cast<std::uintptr_t>(bindings.slot)!=bindings.controller_vtable+0x60 ||
            !bindings.api.set_hidden || !bindings.api.set_frame || !bindings.api.frame ||
            !bindings.api.animation || !bindings.api.play) return false;
        bindings_=bindings;
        active_=this;
        if(slot_.install(bindings.slot,reinterpret_cast<void*>(bindings.original),
            reinterpret_cast<void*>(&callback))) return true;
        // Protection restoration may fail after the replacement was published.
        // Keep its owner until explicit restore succeeds in that case.
        if(!slot_.active()) active_=nullptr;
        return false;
    }
    bool restore() {
        if(!slot_.restore()) return false;
        if(active_==this) active_=nullptr;
        return true;
    }
};
} // namespace stormslinger
