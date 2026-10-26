#pragma once
#include "stormbolt_instance_store.h"
#include "stormbolt_native_memory.h"
#include "hooks.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace stormslinger {
// PS snapshot44/20. Never append these fields to the original PC 0x40-byte object.
struct StormboltSwapEnergyState {float saved=-1.0f,elapsed=0.0f;};
inline void advance_stormbolt_swap_energy(StormboltSwapEnergyState& s,float step,float focus) noexcept {
    s.elapsed=(std::min)(s.elapsed+step*focus,10000.0f);
}
inline float restore_stormbolt_swap_energy(const StormboltSwapEnergyState& s,float maximum,float rate) noexcept {
    return s.saved>=0.0f ? (std::min)(s.saved+rate*s.elapsed,maximum) : maximum;
}
struct StormboltSwapEnergyBindings {
    void* (*construct)(void*)=nullptr;
    void (*reset)(void*)=nullptr;
    void* (*destroy)(void*,std::uint32_t)=nullptr;
    void (*tick)(void*,float)=nullptr;
    void (*save)(void*)=nullptr;
    void (*restore)(void*)=nullptr;
    float (*focus)(void*)=nullptr;
    std::uintptr_t snapshot_vtable=0,weapon_vtable=0,parameter_vtable=0;
};
template<std::size_t Capacity>
class StormboltSwapEnergyHooks {
    using Store=StormboltInstanceStore<Capacity,StormboltSwapEnergyState>;
    Store store_;
    StormboltSwapEnergyBindings bindings_{},native_{};
    bool enabled_=false;
    inline static StormboltSwapEnergyHooks* owner_=nullptr;
    bool snapshot(void* p,StormboltInstanceToken& token) {
        std::uintptr_t vt=0;const auto a=reinterpret_cast<std::uintptr_t>(p);
        return read_stormbolt_field(a,0,vt) && vt==bindings_.snapshot_vtable && store_.token_for_live_address(a,token);
    }
    void retire(void* p) {
        StormboltInstanceToken t{};
        if(store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(p),t)) store_.retire(t);
    }
    bool weapon(void* p,std::uintptr_t& state,std::uintptr_t& parameter) {
        const auto w=reinterpret_cast<std::uintptr_t>(p);std::uintptr_t vt=0,actor=0;std::uint8_t flags=0;
        return read_stormbolt_field(w,0,vt) && vt==bindings_.weapon_vtable &&
            read_stormbolt_field(w,0x9B0,actor) && read_stormbolt_field(actor,0xC,flags) && (flags&0xE) &&
            read_stormbolt_field(actor,0xEEB8,state) && read_stormbolt_field(actor,0x7810,parameter) &&
            read_stormbolt_field(parameter,0,vt) && vt==bindings_.parameter_vtable;
    }
    static void* construct(void* p) {
        auto& s=*owner_;if(s.enabled_)s.retire(p);
        void* r=s.native_.construct(p);std::uintptr_t vt=0;
        if(s.enabled_ && r==p && read_stormbolt_field(reinterpret_cast<std::uintptr_t>(p),0,vt) && vt==s.bindings_.snapshot_vtable)
            s.store_.initialize(reinterpret_cast<std::uintptr_t>(p));
        return r;
    }
    static void reset(void* p) {
        auto& s=*owner_;StormboltInstanceToken t{};const bool tracked=s.enabled_ && s.snapshot(p,t);
        if(tracked)s.store_.retire(t);
        s.native_.reset(p);
        if(tracked)s.store_.initialize(reinterpret_cast<std::uintptr_t>(p));
    }
    static void* destroy(void* p,std::uint32_t flags) {
        auto& s=*owner_;if(s.enabled_)s.retire(p);return s.native_.destroy(p,flags);
    }
    static void tick(void* p,float step) {
        auto& s=*owner_;s.native_.tick(p,step);
        StormboltInstanceToken t{};std::uintptr_t actor=0,status=0;
        if(!s.enabled_ || !s.snapshot(p,t) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(p),8,actor) ||
            !read_stormbolt_field(actor,0x7D20,status))return;
        const float factor=s.bindings_.focus(reinterpret_cast<void*>(status));
        s.store_.transact(t,[&](auto& value) noexcept {advance_stormbolt_swap_energy(value,step,factor);return true;});
    }
    static void save(void* p) {
        auto& s=*owner_;std::uintptr_t state=0,param=0;std::uint8_t skip=1;float gauge=0;StormboltInstanceToken t{};
        if(s.enabled_ && s.weapon(p,state,param) &&
            read_stormbolt_field(reinterpret_cast<std::uintptr_t>(p),0x21D0,skip) && !skip &&
            s.snapshot(reinterpret_cast<void*>(state),t) &&
            read_stormbolt_field(reinterpret_cast<std::uintptr_t>(p),0x2358,gauge))
            s.store_.transact(t,[&](auto& value) noexcept {value.saved=gauge;value.elapsed=0;return true;});
        s.native_.save(p); // Original mine state and tail delegation, exactly once.
    }
    static void restore(void* p) {
        auto& s=*owner_;s.native_.restore(p);
        std::uintptr_t state=0,param=0;float maximum=0,rate=0;StormboltInstanceToken t{};StormboltSwapEnergyState value{};
        if(!s.enabled_ || !s.weapon(p,state,param) || !s.snapshot(reinterpret_cast<void*>(state),t) ||
            !s.store_.snapshot(t,value) || !read_stormbolt_field(param,0x9C0,maximum) ||
            !read_stormbolt_field(param,0x9CC,rate) ||
            !stormbolt_native_span(reinterpret_cast<std::uintptr_t>(p),0x2358,sizeof(float),true))return;
        const float gauge=restore_stormbolt_swap_energy(value,maximum,rate);
        std::memcpy(static_cast<unsigned char*>(p)+0x2358,&gauge,sizeof(gauge));
    }
public:
    template<class Transaction>
    MH_STATUS prepare(Transaction& transaction,const StormboltSwapEnergyBindings& bindings) {
        if(owner_ || !bindings.construct || !bindings.reset || !bindings.destroy || !bindings.tick || !bindings.save ||
            !bindings.restore || !bindings.focus || !bindings.snapshot_vtable || !bindings.weapon_vtable || !bindings.parameter_vtable)
            return MH_ERROR_NOT_INITIALIZED;
        bindings_=bindings;owner_=this;enabled_=true;
#define STORM_SWAP_ADD(member) do {auto code=transaction.add(reinterpret_cast<void*>(bindings.member),reinterpret_cast<void*>(&member),reinterpret_cast<void**>(&native_.member));if(code!=MH_OK)return code;} while(false)
        STORM_SWAP_ADD(construct);STORM_SWAP_ADD(reset);STORM_SWAP_ADD(destroy);STORM_SWAP_ADD(tick);STORM_SWAP_ADD(save);STORM_SWAP_ADD(restore);
#undef STORM_SWAP_ADD
        return MH_OK;
    }
    void withdraw_while_quiescent() noexcept {enabled_=false;}
    void release_after_stop() noexcept {if(owner_==this)owner_=nullptr;}
};
}
