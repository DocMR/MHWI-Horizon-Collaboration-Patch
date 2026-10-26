#pragma once
#include "stormbolt_action_fields.h"
#include "stormbolt_special_to_normal_adapter.h"

namespace stormslinger {
struct StormboltMiddlePrepareApi {
    StormboltSpecialToNormalApi animation{};
    float (*difference)(void* actor) = nullptr; // 1F62480: B220-B224
    void (*rate)(void* actor,std::uint32_t layer,float value) = nullptr; // 1F72CA0
};
enum class StormboltMiddlePrepareResult { unavailable, waiting, invalidated, armed };
// PS3A11C30..3A11CCC phase0 only. Setting phase1 must NOT fall through to
// consume a firing event in the same update. Outer flags/entry/completion remain.
template<std::size_t Capacity>
class StormboltMiddlePrepareAdapter {
    StormboltInstanceStore<Capacity>& store_;
    StormboltActionFields<Capacity>* fields_=nullptr;
    const StormboltMiddlePrepareApi api_;
    template<class T> static T read(const void* object,std::size_t offset) {
        T value{}; std::memcpy(&value,static_cast<const std::byte*>(object)+offset,sizeof(value)); return value;
    }
    static bool live(void* object) { return object && (read<std::uint8_t>(object,0xC)&0xE); }
    template<class T> T action_read(void* action,std::size_t offset) const {
        if(fields_) return fields_->template get<T>(action,offset);
        T value{};std::memcpy(&value,static_cast<const std::byte*>(action)+offset,sizeof(value));return value;
    }
    template<class T> void action_write(void* action,std::size_t offset,T value) {
        if(fields_) fields_->put(action,offset,value);
        else std::memcpy(static_cast<std::byte*>(action)+offset,&value,sizeof(value));
    }
public:
    StormboltMiddlePrepareAdapter(StormboltInstanceStore<Capacity>& store,const StormboltMiddlePrepareApi& api,StormboltActionFields<Capacity>* fields=nullptr)
        : store_(store),fields_(fields),api_(api) { }
    StormboltMiddlePrepareResult apply(void* action) {
        if (!action || !api_.difference || !api_.rate || !api_.animation.motion)
            return StormboltMiddlePrepareResult::unavailable;
        if (action_read<std::uint32_t>(action,0x30)!=0) return StormboltMiddlePrepareResult::waiting;
        auto* actor=read<void*>(action,0x28);
        if (!live(actor)) return StormboltMiddlePrepareResult::unavailable;
        auto* weapon=read<void*>(actor,0x76B0);
        if (!live(weapon) || read<std::uint32_t>(weapon,0x9F8)!=13)
            return StormboltMiddlePrepareResult::unavailable;
        const auto id=read<std::uint32_t>(weapon,0x9FC);
        StormboltInstanceToken token{};
        if ((id!=238 && id!=239) || !store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token))
            return StormboltMiddlePrepareResult::unavailable;
        // Ordered <= matches PS ucomiss followed by jb, including NaN waiting.
        if (!(std::fabs(api_.difference(actor))<=30.0f)) return StormboltMiddlePrepareResult::waiting;
        for (std::uint32_t layer=0;layer<3;++layer) {
            api_.animation.motion(actor,0x200u+layer,0xC0F4u-layer,layer<2 ? 1u : 3u,0,-1.0f,2.0f);
            if (layer<2) {
                actor=read<void*>(action,0x28);
                if (!live(actor)) return StormboltMiddlePrepareResult::invalidated;
            }
        }
        StormboltStageState ignored{};
        if (!store_.snapshot(token,ignored)) return StormboltMiddlePrepareResult::invalidated;
        auto* owner=read<void*>(weapon,0x9B0);
        if (!live(owner)) return StormboltMiddlePrepareResult::invalidated;
        auto* parameter=read<void*>(owner,0x7810);
        actor=read<void*>(action,0x28);
        if (!parameter || !live(actor)) return StormboltMiddlePrepareResult::invalidated;
        api_.rate(actor,1,read<float>(parameter,0x840));
        const std::uint32_t phase=1;
        action_write(action,0x30,phase);
        return StormboltMiddlePrepareResult::armed;
    }
};
} // namespace stormslinger
