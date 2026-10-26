#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_instance_store.h"
#include <cstring>

namespace stormslinger {
struct StormboltMiddleFireApi {
    void* (*event_object)(void* actor) = nullptr; // 1F62070
    const void* (*origin)(void* actor) = nullptr; // 1F62510
    void (*fire)(void* weapon, const void* origin, bool arg3,
        std::uint32_t arg4, bool arg5) = nullptr; // 214BC20, return ignored
};
enum class StormboltMiddleFireResult { unavailable, waiting, invalidated, returned };

// PS3A11CDB..3A11DA9 phase1 event/fire branch only. Entry, phase0 motion
// preparation, actor flags and completion query remain the enclosing action's
// responsibility. Call only with authenticated APIs/types/resources and borrowed
// native lifetimes/serialized access. Never replay a consumed event on failure.
template<std::size_t Capacity>
class StormboltMiddleFireAdapter {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltMiddleFireApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{}; std::memcpy(&value,static_cast<const std::byte*>(object)+offset,sizeof(value)); return value;
    }
    template<class T> static void write(void* object, std::size_t offset, T value) {
        std::memcpy(static_cast<std::byte*>(object)+offset,&value,sizeof(value));
    }
    static bool live(const void* object) { return object && (read<std::uint8_t>(object,0xC)&0xE); }
    template<class T> T action_read(void* action,std::size_t offset) const {
        if(fields_) return fields_->template get<T>(action,offset);
        T value{};std::memcpy(&value,static_cast<const std::byte*>(action)+offset,sizeof(value));return value;
    }
    template<class T> void action_write(void* action,std::size_t offset,T value) {
        if(fields_) fields_->put(action,offset,value);
        else std::memcpy(static_cast<std::byte*>(action)+offset,&value,sizeof(value));
    }
public:
    StormboltMiddleFireAdapter(StormboltInstanceStore<Capacity>& store, const StormboltMiddleFireApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        : store_(store),fields_(fields), api_(api) { }
    StormboltMiddleFireResult apply(void* action) {
        if (!action || !api_.event_object || !api_.origin || !api_.fire)
            return StormboltMiddleFireResult::unavailable;
        if (action_read<std::uint32_t>(action,0x30)!=1) return StormboltMiddleFireResult::waiting;
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return StormboltMiddleFireResult::unavailable;
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13)
            return StormboltMiddleFireResult::unavailable;
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token))
            return StormboltMiddleFireResult::unavailable;
        auto* event=api_.event_object(actor);
        if (!event) return StormboltMiddleFireResult::invalidated;
        auto flags=read<std::uint32_t>(event,0x190);
        if (!(flags&1)) return StormboltMiddleFireResult::waiting;
        write(event,0x190,flags&~std::uint32_t{1});
        auto* owner=read<void*>(weapon,0x9B0);
        if (!live(owner)) return StormboltMiddleFireResult::invalidated;
        auto* parameter=read<void*>(owner,0x7810);
        if (!parameter) return StormboltMiddleFireResult::invalidated;
        StormboltStageTiming timing{};
        for (std::size_t row=0; row<9; ++row)
            timing.action_row[row]=read<float>(parameter,0x1608+row*40);
        timing.post_shot_decay=read<float>(parameter,0x1768);
        if (!store_.transact(token,[&](auto& state) noexcept { return advance_stormbolt_stage(state,timing); }))
            return StormboltMiddleFireResult::invalidated;
        const auto* origin=api_.origin(actor);
        StormboltStageState current{};
        if (!origin || !store_.snapshot(token,current)) return StormboltMiddleFireResult::invalidated;
        api_.fire(weapon,origin,true,0xFFFFFFFF,true);
        action_write(action,0x34,std::uint8_t{1}); // Not proof a projectile was created.
        return StormboltMiddleFireResult::returned;
    }
};
} // namespace stormslinger
