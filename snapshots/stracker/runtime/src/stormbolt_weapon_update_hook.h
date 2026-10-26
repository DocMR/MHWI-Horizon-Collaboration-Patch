#pragma once
#include "stormbolt_weapon_update.h"
#include "owned_vtable_pointer.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
struct StormboltWeaponUpdateBindings {
    std::uintptr_t weapon_vtable=0;
    StormboltWeaponUpdate original=nullptr;
    bool (*ready)(void*)=nullptr; // Required post-original native/resource preflight.
};
// Install/restore only under external quiescence. Keep this object and all
// borrowed API dependencies alive until restoration succeeds.
template<std::size_t Capacity>
class StormboltWeaponUpdateHook {
    StormboltInstanceStore<Capacity>& store_;
    StormboltStageUpdateApi api_;
    StormboltWeaponUpdateBindings bindings_{};
    std::uint32_t base_color_;
    std::array<float,4> low_,high_;
    OwnedVtablePointer<> pointer_;
    inline static StormboltWeaponUpdateHook* owner_=nullptr;
    bool selected(void* weapon) {
        const auto address=reinterpret_cast<std::uintptr_t>(weapon);
        std::uintptr_t table{};std::uint8_t alive{};
        struct Identity {std::uint32_t category,id;} identity{};
        StormboltInstanceToken token{};
        return read_stormbolt_field(address,0,table) && table==bindings_.weapon_vtable &&
            read_stormbolt_field(address,0xC,alive) && (alive&0xE) &&
            read_stormbolt_field(address,0x9F8,identity) && identity.category==13 &&
            (identity.id==238 || identity.id==239) && store_.token_for_live_address(address,token);
    }
    static bool ready_after_original(void* weapon) {
        auto& hook=*owner_;
        // Native update and resource preflight are outside the sidecar lock.
        // Recheck identity across both calls; token generation is independently
        // checked by the wrapper. Neither check establishes external lifetime.
        if (!hook.selected(weapon) || !hook.bindings_.ready(weapon)) return false;
        return hook.selected(weapon);
    }
    static void callback(void* weapon) {
        auto& hook=*owner_;
        if(!hook.selected(weapon)) {hook.bindings_.original(weapon);return;}
        stormbolt_update_with_original(hook.store_,weapon,hook.bindings_.original,hook.api_,
            hook.base_color_,hook.low_,hook.high_,&ready_after_original);
    }
public:
    StormboltWeaponUpdateHook(StormboltInstanceStore<Capacity>& store,const StormboltStageUpdateApi& api,
        std::uint32_t color,const std::array<float,4>& low,const std::array<float,4>& high)
        :store_(store),api_(api),base_color_(color),low_(low),high_(high) {}
    StormboltWeaponUpdateHook(const StormboltWeaponUpdateHook&)=delete;
    bool active() const noexcept {return pointer_.active();}
    bool install(const StormboltWeaponUpdateBindings& bindings) {
        if(owner_ || !bindings.original || !bindings.ready || !bindings.weapon_vtable ||
           bindings.weapon_vtable>(std::numeric_limits<std::uintptr_t>::max)()-0x228) return false;
        bindings_=bindings;owner_=this;
        const bool installed=pointer_.install(reinterpret_cast<void*>(bindings.weapon_vtable+0x228),
            reinterpret_cast<void*>(bindings.original),reinterpret_cast<void*>(&callback));
        if(!installed && !pointer_.active()) owner_=nullptr;
        return installed;
    }
    bool restore() {
        if(!pointer_.restore()) return false;
        if(owner_==this) owner_=nullptr;
        return true;
    }
};
} // namespace stormslinger
