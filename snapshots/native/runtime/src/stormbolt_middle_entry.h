#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
struct StormboltMiddleEntryApi {
    void (*mode)(void*,std::uint32_t) = nullptr; // PC2044850
    // PC wrapper returns true; PS entry ignores that result.
    bool (*needs_parameter)(void*) = nullptr;
    bool (*parameter)(void*,float,std::uint32_t) = nullptr;
    // Native message field access; binding requires reviewed PS/PC correspondence.
    std::uint8_t (*read_stage)(void*) = nullptr;
    void (*write_stage)(void*,std::uint8_t) = nullptr;
};
// PS3A11B10 entry ordering. Native lifetimes and APIs are caller-authenticated;
// sidecar tokens do not establish native lifetime. No original replay on failure.
template<std::size_t Capacity>
class StormboltMiddleEntry {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltMiddleEntryApi api_;
    template<class T> static T read(const void* p,std::size_t offset) {
        T v{}; std::memcpy(&v,static_cast<const std::byte*>(p)+offset,sizeof(v)); return v;
    }
    template<class T> static void write(void* p,std::size_t offset,T v) {
        std::memcpy(static_cast<std::byte*>(p)+offset,&v,sizeof(v));
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
    StormboltMiddleEntry(StormboltInstanceStore<Capacity>& store,const StormboltMiddleEntryApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        : store_(store),fields_(fields),api_(api) { }
    StormboltSpecialActionResult enter(void* action) {
        if (!action || !api_.mode || !api_.needs_parameter || !api_.parameter || !api_.read_stage || !api_.write_stage) return {};
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return {};
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13) return {};
        auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return {};
        api_.mode(actor,1);
        actor=read<void*>(action,0x28);
        if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
        if (api_.needs_parameter(actor)) {
            StormboltStageState state{};
            if (!store_.snapshot(token,state)) return {StormboltSpecialActionStatus::invalidated,false};
            auto* owner=read<void*>(weapon,0x9B0);
            if (!live(owner)) return {StormboltSpecialActionStatus::invalidated,false};
            auto* parameter=read<void*>(owner,0x7810);
            if (!parameter) return {StormboltSpecialActionStatus::invalidated,false};
            api_.parameter(actor,read<float>(parameter,0x818),2);
            actor=read<void*>(action,0x28);
            if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
        }
        action_write(action,0x30,std::uint32_t{0});
        if (read<std::uint8_t>(actor,0x60)&1) {
            if (read<std::uint32_t>(actor,0x8938)&1) {
                StormboltStageState state{};
                if (!store_.snapshot(token,state)) return {StormboltSpecialActionStatus::invalidated,false};
                api_.write_stage(actor,static_cast<std::uint8_t>(state.counter));
            } else {
                const auto value=api_.read_stage(actor);
                if (!store_.transact(token,[value](auto& state) noexcept { state.counter=value; return true; }))
                    return {StormboltSpecialActionStatus::invalidated,false};
            }
        }
        action_write(action,0x34,std::uint8_t{0});
        return {StormboltSpecialActionStatus::processed,false};
    }
};
} // namespace stormslinger
