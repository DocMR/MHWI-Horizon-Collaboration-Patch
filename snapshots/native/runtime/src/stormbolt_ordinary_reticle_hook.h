#pragma once
#include "stormbolt_charge_hud_hook.h"

namespace stormslinger {
struct StormboltOrdinaryReticleBindings {
    std::uintptr_t vtable{};
    bool (*original)(void*){};
    bool (*input)(void*,StormboltChargeHudInput&){};
};
// PC346EEC0 virtual2F8 is consulted by native display10B4CC0.
// Suppress only the ordinary bowgun GUI's admission in battery arrow mode.
class StormboltOrdinaryReticleHook {
    StormboltOrdinaryReticleBindings bindings_{};
    OwnedVtablePointer<> slot_;
    inline static StormboltOrdinaryReticleHook* active_{};
    static bool callback(void* gui) {
        auto& self=*active_;
        if(!self.bindings_.original(gui))return false;
        StormboltChargeHudInput input{};
        return !self.bindings_.input(gui,input) ||
            !(input.special_ammo_selected && input.weapon_drawn);
    }
public:
    bool install(const StormboltOrdinaryReticleBindings& bindings) {
        if(active_ || !bindings.vtable || !bindings.original || !bindings.input)return false;
        bindings_=bindings;active_=this;
        if(slot_.install(reinterpret_cast<void*>(bindings.vtable+0x2F8),
            reinterpret_cast<void*>(bindings.original),reinterpret_cast<void*>(&callback)))return true;
        if(!slot_.active())active_=nullptr;
        return false;
    }
    bool restore() {
        if(!slot_.restore())return false;
        if(active_==this)active_=nullptr;
        return true;
    }
};
}
