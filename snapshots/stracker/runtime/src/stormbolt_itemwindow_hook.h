#pragma once
#include "stormbolt_charge_hud_hook.h"

namespace stormslinger {
struct StormboltItemWindowBindings {
    std::uintptr_t vtable{};
    void (*original)(void*){};
    bool (*input)(void*,StormboltChargeHudInput&){};
    StormboltChargeHudApi api{};
};
// PS43A3ECB and43A50F5..51ED. These existing bow_change children display the
// arrow-mode shell; they are separate from the charging controller's gauge.
class StormboltItemWindowHook {
    StormboltItemWindowBindings bindings_{};
    OwnedVtablePointer<> slot_;
    inline static StormboltItemWindowHook* active_{};
    static void* node(void* object,std::size_t offset) {
        void* result{};std::memcpy(&result,static_cast<unsigned char*>(object)+offset,sizeof(result));
        return result;
    }
    static void callback(void* window) {
        auto& self=*active_;
        self.bindings_.original(window);
        StormboltChargeHudInput input{};
        if(!self.bindings_.input(window,input)) return;
        auto* change=node(window,0x3CB0);
        auto* bow=node(window,0x3CC0);
        auto* light=node(window,0x3CC8);
        auto* arrow=node(window,0x3CD0);
        auto* gauge_layout=node(window,0x3CA8);
        auto* ammo_layout=node(window,0x3CB8);
        if(!change || !bow || !light || !arrow || !gauge_layout || !ammo_layout) return;
        // PS43A3987 selects the outer mode flag from ammo40 independently of
        // draw state. PS43A41AB..4205 applies that flag to3578/3588; PC's
        // corresponding nodes3CA8/3CB8 still receive its ordinary-mode frame
        // at1E2BE60..BE95. Restore these native layout frames after that update.
        const float layout=input.special_ammo_selected ? 0.0f : 1.0f;
        self.bindings_.api.set_frame(gauge_layout,layout,1);
        self.bindings_.api.set_frame(ammo_layout,layout,1);
        self.bindings_.api.set_hidden(arrow,false);
        self.bindings_.api.set_hidden(bow,!input.weapon_drawn);
        self.bindings_.api.set_hidden(light,true);
        self.bindings_.api.set_frame(change,0.0f,1);
    }
public:
    bool install(const StormboltItemWindowBindings& bindings) {
        if(active_ || !bindings.vtable || !bindings.original || !bindings.input ||
            !bindings.api.set_hidden || !bindings.api.set_frame) return false;
        bindings_=bindings;active_=this;
        if(slot_.install(reinterpret_cast<void*>(bindings.vtable+0x260),
            reinterpret_cast<void*>(bindings.original),reinterpret_cast<void*>(&callback))) return true;
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
