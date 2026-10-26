#pragma once
#include "stormbolt_source_transition_lifecycle.h"
#include "stormbolt_source_transition_predicate.h"
#include "stormbolt_shell_hook.h"
#include "stormbolt_live_probe.h"

namespace stormslinger {
struct StormboltSourceTransitionConditionBindings {
    std::uintptr_t transition_vtable=0,actor_vtable=0,weapon_vtable=0;
    std::int32_t (*original)(void*)=nullptr; // SetBomb slot31C92B0+30 ->F13060.
    bool (*ready)(void*,void*)=nullptr;
    bool (*ready_with_context)(void*,void*,void*)=nullptr;
    void* ready_context=nullptr;
};

// Only extended objects published by the creation/lifecycle component qualify.
// Bootstrap authenticates the semantic API and native borrowed lifetimes.
// Calls never execute under the store lock; original is never replayed after
// entering the decision body and its potentially stateful command callback.
template<std::size_t Capacity>
class StormboltSourceTransitionConditionHook {
    using Store=typename StormboltSourceTransitionLifecycle<Capacity>::Store;
    Store& store_;
    StormboltSourceTransitionPredicateApi api_{};
    StormboltSourceTransitionConditionBindings bindings_{};
    OwnedVtablePointer<> slot_;
    inline static StormboltSourceTransitionConditionHook* owner_=nullptr;
    bool select(void* transition,StormboltInstanceToken& token,void*& weapon) {
        const auto address=reinterpret_cast<std::uintptr_t>(transition);
        if(!store_.token_for_live_address(address,token)) return false;
        std::uintptr_t table{},actor{},native_weapon{},native_owner{};
        std::uint8_t alive{};
        struct Identity {std::uint32_t category,id;} identity{};
        if(!read_stormbolt_field(address,0,table) || table!=bindings_.transition_vtable ||
            !read_stormbolt_field(address,0x10,actor) ||
            !read_stormbolt_field(actor,0,table) || table!=bindings_.actor_vtable ||
            !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(actor,0x76B0,native_weapon) ||
            !read_stormbolt_field(native_weapon,0,table) || table!=bindings_.weapon_vtable ||
            !read_stormbolt_field(native_weapon,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(native_weapon,0x9B0,native_owner) || native_owner!=actor ||
            !read_stormbolt_field(native_weapon,0x9F8,identity) || identity.category!=13 ||
            (identity.id!=238 && identity.id!=239)) return false;
        weapon=reinterpret_cast<void*>(native_weapon);
        return true;
    }
    static std::int32_t callback(void* transition) {
        stormbolt_live_probe.source_calls.fetch_add(1,std::memory_order_relaxed);
        stormbolt_live_probe.source_stage.store(1,std::memory_order_relaxed);
        auto& self=*owner_;
        StormboltInstanceToken token{};
        void* weapon{};
        if(!self.select(transition,token,weapon))
            return self.bindings_.original(transition);
        stormbolt_live_probe.source_stage.store(2,std::memory_order_relaxed);
        if(!(self.bindings_.ready_with_context ? self.bindings_.ready_with_context(transition,weapon,self.bindings_.ready_context) : self.bindings_.ready(transition,weapon)))
            return self.bindings_.original(transition);
        stormbolt_live_probe.source_stage.store(3,std::memory_order_relaxed);
        StormboltInstanceToken current{};void* current_weapon{};
        if(!self.select(transition,current,current_weapon) || current.address!=token.address ||
            current.generation!=token.generation || current_weapon!=weapon)
            return self.bindings_.original(transition);
        stormbolt_live_probe.source_stage.store(4,std::memory_order_relaxed);
        // Without Power Source Mod retain the complete PC SetBomb predicate,
        // including its command/query ordering. Do not enter the ported input
        // and authority branches merely because this is a target weapon.
        const auto part8=self.api_.source_count(weapon);
        stormbolt_live_probe.source_part8.store(part8,std::memory_order_relaxed);
        stormbolt_live_probe.source_stage.store(5,std::memory_order_relaxed);
        if(!part8) return self.bindings_.original(transition);
        StormboltSourceTransitionObservation observation{};
        const auto result=stormbolt_evaluate_source_transition(transition,weapon,self.api_,&observation);
        if(observation.direct) stormbolt_live_probe.source_direct_calls.fetch_add(1,std::memory_order_relaxed);
        if(observation.command_called) stormbolt_live_probe.source_command_calls.fetch_add(1,std::memory_order_relaxed);
        if(observation.command_called && observation.command_result!=-1)
            stormbolt_live_probe.source_command_accepted.fetch_add(1,std::memory_order_relaxed);
        if(result==2) stormbolt_live_probe.source_result2_calls.fetch_add(1,std::memory_order_relaxed);
        if(result==3) stormbolt_live_probe.source_result3_calls.fetch_add(1,std::memory_order_relaxed);
        stormbolt_live_probe.source_result.store(static_cast<std::uint32_t>(result),std::memory_order_relaxed);
        stormbolt_live_probe.source_stage.store(6,std::memory_order_relaxed);
        StormboltSourceTransitionState state{};
        return self.store_.snapshot(token,state) ? result : -1;
    }
public:
    explicit StormboltSourceTransitionConditionHook(Store& store):store_(store) {}
    bool active() const noexcept {return slot_.active();}
    bool install(const StormboltSourceTransitionConditionBindings& bindings,
        const StormboltSourceTransitionPredicateApi& api) {
        if(owner_ || !api.complete() || !bindings.transition_vtable ||
            bindings.transition_vtable>(std::numeric_limits<std::uintptr_t>::max)()-0x30 ||
            !bindings.actor_vtable || !bindings.weapon_vtable || !bindings.original || (!bindings.ready && !bindings.ready_with_context))
            return false;
        bindings_=bindings;api_=api;owner_=this;
        const bool installed=slot_.install(reinterpret_cast<void*>(bindings.transition_vtable+0x30),
            reinterpret_cast<void*>(bindings.original),reinterpret_cast<void*>(&callback));
        if(!installed && !slot_.active()) owner_=nullptr;
        return installed;
    }
    bool restore() {
        if(!slot_.restore()) return false;
        if(owner_==this) owner_=nullptr;
        return true;
    }
};
} // namespace stormslinger
