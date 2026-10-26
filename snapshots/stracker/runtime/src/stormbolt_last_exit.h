#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
// PS39F2E10: guarded reset, followed by flag DB in both native flag banks.
// Caller authenticates action/type, borrowed object lifetime and serialization.
template<std::size_t Capacity>
class StormboltLastExit {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    void (*flag_)(void*,std::uint32_t);
    template<class T> static T read(const void* p,std::size_t offset) {
        T value{}; std::memcpy(&value,static_cast<const std::byte*>(p)+offset,sizeof(value)); return value;
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
    StormboltLastExit(StormboltInstanceStore<Capacity>& store,void (*flag)(void*,std::uint32_t),StormboltActionFields<Capacity>* fields=nullptr)
        : store_(store),fields_(fields),flag_(flag) { }
    StormboltSpecialActionResult exit(void* action) {
        if (!action || !flag_) return {};
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return {};
        if (action_read<std::uint8_t>(action,0x34)) {
            auto* weapon=read<void*>(actor,0x76B0);
            if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13) return {};
            const auto id=read<std::uint32_t>(weapon,0x9FC);
            StormboltInstanceToken token{};
            if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return {};
            auto* owner=read<void*>(weapon,0x9B0);
            if (!live(owner)) return {StormboltSpecialActionStatus::invalidated,false};
            auto* parameter=read<void*>(owner,0x7810);
            if (!parameter) return {StormboltSpecialActionStatus::invalidated,false};
            // Native instructions copy raw DWORDs, including non-finite payloads.
            const auto action_bits=read<std::uint32_t>(parameter,0x1608);
            const auto decay_bits=read<std::uint32_t>(parameter,0x1768);
            if (!store_.transact(token,[&](auto& state) noexcept {
                state.counter=0;
                std::memcpy(&state.action_timer,&action_bits,sizeof(action_bits));
                std::memcpy(&state.decay_timer,&decay_bits,sizeof(decay_bits));
                return true;
            })) return {StormboltSpecialActionStatus::invalidated,false};
        }
        flag_(actor,0xDB);
        return {StormboltSpecialActionStatus::processed,false};
    }
};
} // namespace stormslinger
