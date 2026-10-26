#pragma once
#include "stormbolt_power_cell_lifecycle.h"
#include "stormbolt_shell_hook.h"
#include "stormbolt_live_probe.h"

namespace stormslinger {
// PC11B6450(This,owner)->This;11C34A0(This,preserve_effects:bool)->void;11B68A0(This)->void.
// The inner destructor is called by native virtual0 before native freeing.
template<std::size_t Capacity>
class StormboltStatusLifecycleHooks {
    StormboltPowerCellLifecycle<Capacity> lifecycle_;
    std::uintptr_t vtable_=0;
    void* (*construct_)(void*,void*)=nullptr;
    void (*reset_)(void*,bool)=nullptr;
    void (*destroy_)(void*)=nullptr;
    inline static StormboltStatusLifecycleHooks* active_=nullptr;
    bool authenticate(void* object) const {
        std::uintptr_t live=0;
        return read_stormbolt_field(reinterpret_cast<std::uintptr_t>(object),0,live) && live==vtable_;
    }
    static void observe_native_return(void* object,bool construction) {
        const auto address=reinterpret_cast<std::uintptr_t>(object);
        std::uintptr_t owner=0;
        std::uint8_t player=0;
        std::uint32_t count=0,timer=0;
        if(!read_stormbolt_field(address,8,owner) ||
            !read_stormbolt_field(owner,0x60,player) || !(player&1) ||
            !read_stormbolt_field(address,0x760,count) ||
            !read_stormbolt_field(address,0x764,timer)) return;
        auto& probe=stormbolt_live_probe;
        if(construction) {
            probe.construct_native_count.store(count,std::memory_order_relaxed);
            probe.construct_native_timer_bits.store(timer,std::memory_order_relaxed);
            probe.local_construct_returns.fetch_add(1,std::memory_order_relaxed);
        } else {
            probe.reset_native_count.store(count,std::memory_order_relaxed);
            probe.reset_native_timer_bits.store(timer,std::memory_order_relaxed);
            probe.local_reset_returns.fetch_add(1,std::memory_order_relaxed);
        }
    }
    static void* construct_hook(void* object,void* owner) {
        auto& bridge=*active_;
        auto result=bridge.lifecycle_.construct(object,
            [&] {return bridge.construct_(object,owner);},
            [&](void* p) {return bridge.authenticate(p);}).native_result;
        if(result==object && bridge.authenticate(object)) observe_native_return(object,true);
        return result;
    }
    static void reset_hook(void* object,bool preserve_effects) {
        auto& bridge=*active_;
        // Native DL!=0 skips clearing effect counters/timers. Preserve their sidecar too.
        if(preserve_effects) {bridge.reset_(object,true);return;}
        bridge.lifecycle_.reset(object,[&] {bridge.reset_(object,false);},
            [&](void* p) {return bridge.authenticate(p);});
        if(bridge.authenticate(object)) observe_native_return(object,false);
    }
    static void destroy_hook(void* object) {
        auto& bridge=*active_;
        bridge.lifecycle_.destroy(object,[&] {bridge.destroy_(object);});
    }
public:
    StormboltStatusLifecycleHooks(StormboltPowerCellStore<Capacity>& store,std::uintptr_t vtable)
        :lifecycle_(store),vtable_(vtable) {}
    StormboltStatusLifecycleHooks(const StormboltStatusLifecycleHooks&)=delete;
    StormboltStatusLifecycleHooks& operator=(const StormboltStatusLifecycleHooks&)=delete;
    bool owns_bridge() const noexcept {return active_==this;}
    MH_STATUS add_to_transaction(HookTransaction& transaction,void* construct,void* reset,void* destroy) {
        if (active_) return MH_ERROR_ALREADY_CREATED;
        if (!vtable_ || !construct || !reset || !destroy) return MH_ERROR_NOT_EXECUTABLE;
        active_=this;
#if defined(STORMSLINGER_DIAGNOSTIC_STATUS_LIFECYCLE_MASK)
        constexpr unsigned mask=STORMSLINGER_DIAGNOSTIC_STATUS_LIFECYCLE_MASK;
#else
        constexpr unsigned mask=7;
#endif
        auto code=MH_OK;
        if constexpr(mask&1) code=transaction.add(construct,reinterpret_cast<void*>(&construct_hook),reinterpret_cast<void**>(&construct_));
        if constexpr(mask&2) if (code==MH_OK) code=transaction.add(reset,reinterpret_cast<void*>(&reset_hook),reinterpret_cast<void**>(&reset_));
        if constexpr(mask&4) if (code==MH_OK) code=transaction.add(destroy,reinterpret_cast<void*>(&destroy_hook),reinterpret_cast<void**>(&destroy_));
        return code;
    }
    // Caller has fully stopped its transaction and drained native callbacks.
    bool release_after_unhook() {
        if (active_!=this) return false;
        active_=nullptr;construct_=nullptr;reset_=nullptr;destroy_=nullptr;
        return true;
    }
};
} // namespace stormslinger
