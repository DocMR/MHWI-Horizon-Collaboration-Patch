#pragma once
#include "stormbolt_instance_store.h"
#include <cstring>
namespace stormslinger {
// PS reference fixtures have extension fields at30/34. PC native actions are
// only30 bytes: production cores always provide this sidecar accessor instead.
template<std::size_t Capacity>
class StormboltActionFields {
    StormboltInstanceStore<Capacity>& store_;
    bool token(void* action,StormboltInstanceToken& out) const {
        void* actor{};void* weapon{};
        std::memcpy(&actor,static_cast<const std::byte*>(action)+0x28,8);
        if(!actor) return false;
        std::memcpy(&weapon,static_cast<const std::byte*>(actor)+0x76B0,8);
        return store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),out);
    }
public:
    explicit StormboltActionFields(StormboltInstanceStore<Capacity>& store):store_(store) {}
    template<class T> T get(void* action,std::size_t offset) const {
        StormboltInstanceToken id{};StormboltStageState state{};
        if(!token(action,id) || !store_.snapshot(id,state) || state.action_owner!=reinterpret_cast<std::uintptr_t>(action)) return T{};
        return static_cast<T>(offset==0x30?state.action_phase:state.action_fired);
    }
    template<class T> void put(void* action,std::size_t offset,T value) {
        StormboltInstanceToken id{};if(!token(action,id)) return;
        store_.transact(id,[&](auto& state) noexcept {
            if(state.action_owner!=reinterpret_cast<std::uintptr_t>(action)) {
                state.action_owner=reinterpret_cast<std::uintptr_t>(action);
                state.action_phase=0;state.action_fired=0;
            }
            if(offset==0x30) state.action_phase=static_cast<std::uint32_t>(value);
            else state.action_fired=static_cast<std::uint8_t>(value);
            return true;
        });
    }
};
} // namespace stormslinger
