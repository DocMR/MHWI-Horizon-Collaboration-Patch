#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_middle_prepare_adapter.h"
#include "stormbolt_middle_fire_adapter.h"

namespace stormslinger {
// PS3A11BC0 update only; entry remains separate. Authenticated native actor_flag
// for these IDs only sets the two bit banks (no callbacks/object replacement).
// Caller owns readiness, serialized access and borrowed native lifetimes.
template<std::size_t Capacity>
class StormboltMiddleUpdate {
    StormboltMiddlePrepareAdapter<Capacity> prepare_;
    StormboltMiddleFireAdapter<Capacity> fire_;
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltSpecialToNormalApi api_;
    template<class T> static T read(const void* p,std::size_t o) {
        T v{}; std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v)); return v;
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
    StormboltMiddleUpdate(StormboltInstanceStore<Capacity>& store,
        const StormboltMiddlePrepareApi& prepare,const StormboltMiddleFireApi& fire,
        const StormboltSpecialToNormalApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        : prepare_(store,prepare,fields),fire_(store,fire,fields),store_(store),fields_(fields),api_(api) { }
    StormboltSpecialActionResult update(void* action) {
        if (!action || !api_.actor_flag || !api_.query) return {};
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return {};
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13) return {};
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return {};
        for (const auto flag : {0x5Du,0x22u,0x28u,0x7Eu}) api_.actor_flag(actor,flag);
        const auto phase=action_read<std::uint32_t>(action,0x30);
        if (phase==0) {
            const auto result=prepare_.apply(action);
            if (result==StormboltMiddlePrepareResult::unavailable || result==StormboltMiddlePrepareResult::invalidated)
                return {StormboltSpecialActionStatus::invalidated,false};
        } else if (phase==1) {
            const auto result=fire_.apply(action);
            if (result==StormboltMiddleFireResult::unavailable || result==StormboltMiddleFireResult::invalidated)
                return {StormboltSpecialActionStatus::invalidated,false};
            actor=read<void*>(action,0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
            if (api_.query(actor,0)) return {StormboltSpecialActionStatus::processed,true};
        }
        if (!action_read<std::uint8_t>(action,0x34)) {
            actor=read<void*>(action,0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
            api_.actor_flag(actor,0x16D);
        }
        return {StormboltSpecialActionStatus::processed,false};
    }
};
} // namespace stormslinger
