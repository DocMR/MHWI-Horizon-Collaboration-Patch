#pragma once
#include "stormbolt_shell_hook.h"
#include <intrin.h>

namespace stormslinger {
using StormboltMuzzleEffect=void*(*)(void*,std::uint32_t,std::uint32_t,std::uint32_t);
struct StormboltMuzzleEffectBindings {
    StormboltMuzzleEffect entry{}; // AE27E0: ordinary muzzle resource component+28.
    std::uintptr_t shot_return{},actor_vtable{},weapon_vtable{};
    bool (*ready)(void*){};
    void* context{};
};
// The selected projectile retains its own lbg024_shell EPV and sound requests.
// Only the additional ordinary muzzle request at214CE66 is removed; unrelated
// callers and no-battery/ordinary shots retain the exact original return value.
template<std::size_t Capacity>
class StormboltMuzzleEffectHook {
    StormboltInstanceStore<Capacity>& store_;
    StormboltMuzzleEffectBindings bindings_{};
    StormboltMuzzleEffect original_{};
    bool enabled_=false;
    inline static StormboltMuzzleEffectHook* owner_{};
    bool suppress(void* component,std::uintptr_t caller,std::uint32_t group,std::uint32_t record) {
        if(!enabled_ || caller!=bindings_.shot_return || group!=2000 || record!=0 ||
            (bindings_.ready && !bindings_.ready(bindings_.context))) return false;
        std::uintptr_t actor{},weapon{},table{},back{};
        std::uint8_t alive{};
        std::uint32_t category{},id{},parts{},ammo{};
        const auto address=reinterpret_cast<std::uintptr_t>(component);
        if(!read_stormbolt_field(address,0x18,actor) || !actor ||
            !read_stormbolt_field(actor,0,table) || table!=bindings_.actor_vtable ||
            !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(actor,0x8808,back) || back!=address ||
            !read_stormbolt_field(actor,0x76B0,weapon) || !weapon ||
            !read_stormbolt_field(weapon,0,table) || table!=bindings_.weapon_vtable ||
            !read_stormbolt_field(weapon,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(weapon,0x9B0,back) || back!=actor ||
            !read_stormbolt_field(weapon,0x9F8,category) || category!=13 ||
            !read_stormbolt_field(weapon,0x9FC,id) || (id!=238 && id!=239) ||
            !read_stormbolt_field(weapon,0x2388,parts) || !parts ||
            !read_stormbolt_field(weapon,0x27D0,ammo)) return false;
        if(ammo==0xFF && !read_stormbolt_field(weapon,0x27C8,ammo)) return false;
        StormboltInstanceToken token{};
        return ammo==40 && store_.token_for_live_address(weapon,token);
    }
    static void* callback(void* component,std::uint32_t group,std::uint32_t record,std::uint32_t parameter) {
        auto& self=*owner_;
        if(self.suppress(component,reinterpret_cast<std::uintptr_t>(_ReturnAddress()),group,record)) return nullptr;
        return self.original_(component,group,record,parameter);
    }
public:
    explicit StormboltMuzzleEffectHook(StormboltInstanceStore<Capacity>& store):store_(store) {}
    template<class Transaction>
    MH_STATUS prepare(Transaction& transaction,const StormboltMuzzleEffectBindings& bindings) {
        if(owner_ || !bindings.entry || !bindings.shot_return || !bindings.actor_vtable || !bindings.weapon_vtable)
            return MH_ERROR_NOT_INITIALIZED;
        bindings_=bindings;owner_=this;enabled_=true;
        return transaction.add(reinterpret_cast<void*>(bindings.entry),reinterpret_cast<void*>(&callback),
            reinterpret_cast<void**>(&original_));
    }
    void withdraw_while_quiescent() noexcept {enabled_=false;}
    void release_after_stop() noexcept {if(owner_==this) owner_=nullptr;}
};
}
