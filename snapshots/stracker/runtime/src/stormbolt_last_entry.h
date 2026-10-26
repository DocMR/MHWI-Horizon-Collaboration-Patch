#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
struct StormboltLastEntryApi {
    void (*mode)(void*,std::uint32_t) = nullptr;
    bool (*parameter)(void*,float,std::uint32_t) = nullptr;
    // PS actor virtual290 -> PC virtual288 / 1C014B0.
    void (*request_motion)(void*,std::uint32_t,std::uint32_t) = nullptr;
};
// PS39F2B20. Scoped caller authenticates the action, native lifetimes and APIs.
// Entry does not fire. Native callbacks run outside the sidecar lock.
template<std::size_t Capacity>
class StormboltLastEntry {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltLastEntryApi api_;
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
    StormboltLastEntry(StormboltInstanceStore<Capacity>& store,const StormboltLastEntryApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        :store_(store),fields_(fields),api_(api) { }
    StormboltSpecialActionResult enter(void* action) {
        if (!action || !api_.mode || !api_.parameter || !api_.request_motion) return {};
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return {};
        write(actor,0x82D0,std::uint32_t{1}); // PS80F0 -> PC82D0.
        api_.mode(actor,0);
        action_write(action,0x30,std::uint32_t{0});
        actor=read<void*>(action,0x28);
        if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13)
            return {StormboltSpecialActionStatus::invalidated,false};
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token))
            return {StormboltSpecialActionStatus::invalidated,false};
        auto* owner=read<void*>(weapon,0x9B0);
        if (!live(owner)) return {StormboltSpecialActionStatus::invalidated,false};
        auto* parameter=read<void*>(owner,0x7810);
        if (!parameter) return {StormboltSpecialActionStatus::invalidated,false};
        api_.parameter(actor,read<float>(parameter,0x818),2);
        StormboltStageState current{};
        if (!store_.snapshot(token,current)) return {StormboltSpecialActionStatus::invalidated,false};
        owner=read<void*>(weapon,0x9B0); // PS reloads owner/parameter after helper.
        if (!live(owner)) return {StormboltSpecialActionStatus::invalidated,false};
        parameter=read<void*>(owner,0x7810);
        if (!parameter) return {StormboltSpecialActionStatus::invalidated,false};
        std::uint32_t action_bits{};
        if (current.counter<=8) action_bits=read<std::uint32_t>(parameter,0x1608+40*current.counter);
        const auto decay_bits=read<std::uint32_t>(parameter,0x1768);
        if (!store_.transact(token,[&](auto& state) noexcept {
            if (state.counter!=current.counter) return false;
            if (current.counter<=8) std::memcpy(&state.action_timer,&action_bits,4);
            std::memcpy(&state.decay_timer,&decay_bits,4);
            return true;
        })) return {StormboltSpecialActionStatus::invalidated,false};
        actor=read<void*>(action,0x28);
        if (!live(actor)) return {StormboltSpecialActionStatus::invalidated,false};
        api_.request_motion(actor,0xC0F5,0);
        actor=read<void*>(action,0x28);
        if (!live(actor) || !store_.snapshot(token,current))
            return {StormboltSpecialActionStatus::invalidated,false};
        if (!(read<std::uint8_t>(actor,0x8938)&1)) {
            if (!store_.transact(token,[](auto& state) noexcept { state.counter=8; return true; }))
                return {StormboltSpecialActionStatus::invalidated,false};
        }
        action_write(action,0x34,std::uint8_t{0});
        return {StormboltSpecialActionStatus::processed,false};
    }
};
} // namespace stormslinger
