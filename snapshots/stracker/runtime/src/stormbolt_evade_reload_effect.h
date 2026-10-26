#pragma once
#include "stormbolt_instance_store.h"
#include "stormbolt_native_memory.h"

namespace stormslinger {
struct StormboltEvadeReloadEffectBindings {
    void (*original_refill)(void*)=nullptr;
    void (*weapon_effect)(void*,std::uint32_t,std::uint32_t,std::uint32_t)=nullptr;
    bool (*ready)(void*,void*)=nullptr;
    void* context=nullptr;
    // Supplied only after the runtime resolved-model mapping is authenticated.
    std::size_t resolved_model_offset=0;
};
// Only the audited ExDodge update refill CALL may use this adapter. The caller
// holds the native borrow through refill/effect; no generic refill detour.
template<std::size_t Capacity>
class StormboltEvadeReloadEffect {
    StormboltInstanceStore<Capacity>& store_;
    StormboltEvadeReloadEffectBindings api_;
public:
    StormboltEvadeReloadEffect(StormboltInstanceStore<Capacity>& instances,
        StormboltEvadeReloadEffectBindings api):store_(instances),api_(api) {}
    bool invoke(void* weapon) {
        if(!api_.original_refill) return false;
        const auto address=reinterpret_cast<std::uintptr_t>(weapon);
        StormboltInstanceToken token{};
        const bool tracked=store_.token_for_live_address(address,token);
        api_.original_refill(weapon); // Exactly once, including non-targets.
        StormboltStageState snapshot{};
        if(!tracked || !api_.weapon_effect || !api_.ready || !api_.resolved_model_offset ||
            !store_.snapshot(token,snapshot)) return false;
        if(!api_.ready(weapon,api_.context) || !store_.snapshot(token,snapshot)) return false;
        std::uint32_t category=0,id=0,model=0;std::uint8_t alive=0;
        std::uintptr_t owner=0,provider=0;
        if(!read_stormbolt_field(address,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(address,0x9F8,category) || category!=13 ||
            !read_stormbolt_field(address,0x9FC,id) || (id!=238 && id!=239) ||
            !read_stormbolt_field(address,api_.resolved_model_offset,model) || model!=24 ||
            !read_stormbolt_field(address,0x9B0,owner) ||
            !read_stormbolt_field(owner,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(owner,0x8808,provider)) return false;
        // PS reads the provider and passes it through. Readiness authenticates
        // that dependency; no sidecar lock is held across the native request.
        api_.weapon_effect(reinterpret_cast<void*>(provider),2000,0,0xFFFFFFFF);
        return true;
    }
};
}
