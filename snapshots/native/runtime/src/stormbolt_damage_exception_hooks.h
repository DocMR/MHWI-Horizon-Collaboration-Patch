#pragma once
#include "stormbolt_damage_exception_store.h"
#include "hooks.h"

namespace stormslinger {
template<std::size_t Capacity>
class StormboltDamageExceptionHooks {
    StormboltDamageExceptionStore<Capacity>& store_;
    std::uintptr_t vtable_;
    void (*update_)(void*)=nullptr;
    float (*absorb_)(void*,float)=nullptr;
    inline static StormboltDamageExceptionHooks* active_=nullptr;
    static void update_hook(void* component) {
        auto& bridge=*active_;
        StormboltDamageExceptionToken token{};
        std::uintptr_t vtable=0;
        const auto address=reinterpret_cast<std::uintptr_t>(component);
        const bool tracked=read_stormbolt_field(address,0,vtable) && vtable==bridge.vtable_ &&
            bridge.store_.token_for_live_address(address,token);
        bridge.update_(component);
        if (tracked) (void)clear_stormbolt_damage_exception_after_update(token,bridge.store_);
    }
    static float absorb_hook(void* stateObject,float input) {
        auto& bridge=*active_;
        StormboltDamageExceptionToken token{};
        const bool tracked=capture_stormbolt_damage_exception_for_absorption(
            stateObject,bridge.vtable_,bridge.store_,token);
        const float result=bridge.absorb_(stateObject,input);
        if (tracked) (void)mark_stormbolt_damage_exception_for_captured_absorption(token,input,bridge.store_);
        return result;
    }
public:
    StormboltDamageExceptionHooks(StormboltDamageExceptionStore<Capacity>& store,std::uintptr_t vtable)
        :store_(store),vtable_(vtable) {}
    bool owns_bridge() const noexcept {return active_==this;}
    MH_STATUS add_to_transaction(HookTransaction& transaction,void* update,void* absorb) {
        if (active_) return MH_ERROR_ALREADY_CREATED;
        if (!vtable_ || !update || !absorb) return MH_ERROR_NOT_EXECUTABLE;
        active_=this;
        auto code=transaction.add(update,reinterpret_cast<void*>(&update_hook),reinterpret_cast<void**>(&update_));
        if (code==MH_OK) code=transaction.add(absorb,reinterpret_cast<void*>(&absorb_hook),reinterpret_cast<void**>(&absorb_));
        return code;
    }
    // Caller restores/stops the shared transaction and drains native frames.
    bool release_after_unhook() {
        if (active_!=this) return false;
        active_=nullptr;update_=nullptr;absorb_=nullptr;return true;
    }
};
} // namespace stormslinger
