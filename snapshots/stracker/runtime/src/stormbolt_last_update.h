#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_special_to_normal_adapter.h"
#include <cmath>

namespace stormslinger {
struct StormboltLastUpdateApi {
    void* (*event_object)(void*) = nullptr;
    float (*difference)(void*) = nullptr;
    const void* (*origin)(void*) = nullptr;
    void (*fire)(void*,const void*,bool,std::uint32_t,bool) = nullptr;
    void (*actor_flag)(void*,std::uint32_t) = nullptr;
    // The effect adapter must check all four native references, request 7DD only
    // when inactive and bind the returned reference. Native mapping is pending.
    bool (*effect_event)(void*) = nullptr;
    bool (*complete)(void*) = nullptr; // PS virtual2B8, not Middle's channel query.
    const void* effect_context = nullptr;
    bool (*effect_with_context)(const void*,void*) = nullptr;
};
// PS39F2C00. Scoped caller supplies authenticated types, native lifetimes,
// serialized state access and readiness. No replay after a consumed event.
template<std::size_t Capacity>
class StormboltLastUpdate {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltLastUpdateApi api_;
    template<class T> static T read(const void* p,std::size_t o) {
        T v{}; std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v)); return v;
    }
    template<class T> static void write(void* p,std::size_t o,T v) {
        std::memcpy(static_cast<std::byte*>(p)+o,&v,sizeof(v));
    }
    static bool live(void* p) { return p && (read<std::uint8_t>(p,0xC)&0xE); }
    template<class T> T action_read(void* action,std::size_t offset) const {
        if(fields_) return fields_->template get<T>(action,offset);
        T value{};std::memcpy(&value,static_cast<const std::byte*>(action)+offset,sizeof(value));return value;
    }
    template<class T> void action_write(void* action,std::size_t offset,T value) {
        if(fields_) fields_->put(action,offset,value);
        else std::memcpy(static_cast<std::byte*>(action)+offset,&value,sizeof(value));
    }
public:
    StormboltLastUpdate(StormboltInstanceStore<Capacity>& store,const StormboltLastUpdateApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        :store_(store),fields_(fields),api_(api) { }
    StormboltSpecialActionResult update(void* action) {
        if (!action || !api_.event_object || !api_.difference || !api_.origin || !api_.fire ||
            !api_.actor_flag || !(api_.effect_with_context ? api_.effect_context!=nullptr : api_.effect_event!=nullptr) || !api_.complete) return {};
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return {};
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13) return {};
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return {};
        auto* event=api_.event_object(actor); // Retain initial event for both bits.
        if (!event) return {StormboltSpecialActionStatus::invalidated,false};
        // PS39F2C7B/39F2D5C use the global event bank+28. Middle uses
        // bank+190; sharing its accessor does not make the banks equivalent.
        const auto phase=action_read<std::uint32_t>(action,0x30);
        if (phase==0) {
            if (std::fabs(api_.difference(actor))<=30.0f) action_write(action,0x30,std::uint32_t{1});
        } else if (phase==1) {
            const auto bits=read<std::uint32_t>(event,0x28);
            if (bits&1) {
                write(event,0x28,bits&~std::uint32_t{1});
                StormboltStageState before{};
                if (!store_.snapshot(token,before)) return {StormboltSpecialActionStatus::invalidated,false};
                auto* owner=read<void*>(weapon,0x9B0);
                if (!live(owner)) return {StormboltSpecialActionStatus::invalidated,false};
                auto* parameter=read<void*>(owner,0x7810);
                if (!parameter) return {StormboltSpecialActionStatus::invalidated,false};
                const std::uint32_t next=before.counter+1;
                std::uint32_t action_bits{};
                if (next<=8) action_bits=read<std::uint32_t>(parameter,0x1608+40*next);
                const auto decay_bits=read<std::uint32_t>(parameter,0x1768);
                if (!store_.transact(token,[&](auto& state) noexcept {
                    if (state.counter!=before.counter) return false;
                    state.shot_snapshot=state.counter; state.counter=next;
                    if (next<=8) std::memcpy(&state.action_timer,&action_bits,4);
                    std::memcpy(&state.decay_timer,&decay_bits,4);
                    return true;
                })) return {StormboltSpecialActionStatus::invalidated,false};
                const auto* point=api_.origin(actor);
                if (!point || !store_.snapshot(token,before)) return {StormboltSpecialActionStatus::invalidated,false};
                api_.fire(weapon,point,true,0xFFFFFFFF,true);
                action_write(action,0x34,std::uint8_t{1});
            }
        }
        if (!action_read<std::uint8_t>(action,0x34)) {
            actor=read<void*>(action,0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
            api_.actor_flag(actor,0x16D);
        }
        const auto bits=read<std::uint32_t>(event,0x28);
        if (bits&0x40000000) {
            write(event,0x28,bits&~std::uint32_t{0x40000000});
            actor=read<void*>(action,0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
            const bool handled=api_.effect_with_context ? api_.effect_with_context(api_.effect_context,actor) : api_.effect_event(actor);
            if (!handled) return {StormboltSpecialActionStatus::invalidated,false};
        }
        actor=read<void*>(action,0x28);
        if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
        return {StormboltSpecialActionStatus::processed,api_.complete(actor)};
    }
};
} // namespace stormslinger
