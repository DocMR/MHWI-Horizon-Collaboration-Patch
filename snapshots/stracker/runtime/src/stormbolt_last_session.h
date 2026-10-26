#pragma once
#include "stormbolt_last_entry.h"
#include "stormbolt_last_update.h"
#include "stormbolt_last_exit.h"
#include "stormbolt_special_to_normal_hook.h"

namespace stormslinger {
template<std::size_t Capacity>
class StormboltLastCore {
    StormboltActionFields<Capacity> fields_;
    StormboltPlayerEffectAdapter effects_;
    StormboltLastEntry<Capacity> entry_;
    StormboltLastUpdate<Capacity> update_;
    StormboltLastExit<Capacity> exit_;
    static bool effect(const void* context,void* actor) {
        void* component{};
        std::memcpy(&component,static_cast<const std::byte*>(actor)+0x8808,sizeof(component));
        return component && static_cast<const StormboltLastCore*>(context)->effects_.update_last_effect(component);
    }
    static StormboltLastUpdateApi bind(StormboltLastUpdateApi api,const StormboltLastCore* owner) {
        api.effect_event=nullptr; api.effect_context=owner; api.effect_with_context=effect; return api;
    }
public:
    StormboltLastCore(StormboltInstanceStore<Capacity>& store,const StormboltLastEntryApi& entry,
        const StormboltLastUpdateApi& update,const StormboltPlayerEffectApi& effects)
        :fields_(store),effects_(effects),entry_(store,entry,&fields_),update_(store,bind(update,this),&fields_),exit_(store,update.actor_flag,&fields_) { }
    StormboltLastCore(const StormboltLastCore&)=delete;
    StormboltLastCore& operator=(const StormboltLastCore&)=delete;
    auto enter(void* action) { return entry_.enter(action); }
    auto update(void* action) { return update_.update(action); }
    auto exit(void* action) { return exit_.exit(action); }
};
// Stable core/effect ownership. Borrowed provider-resolver contexts and native
// lifetimes must outlive successful quiescent restoration of all three slots.
template<std::size_t Capacity>
class StormboltLastSession {
    StormboltLastCore<Capacity> core_;
    StormboltSpecialToNormalHook<Capacity,StormboltLastCore<Capacity>> hook_;
    const StormboltSpecialToNormalBindings bindings_;
    const bool complete_;
public:
    StormboltLastSession(StormboltInstanceStore<Capacity>& store,const StormboltLastEntryApi& entry,
        const StormboltLastUpdateApi& update,const StormboltPlayerEffectApi& effects,
        const StormboltSpecialToNormalBindings& bindings)
        :core_(store,entry,update,effects),hook_(store,core_),bindings_(bindings),
         complete_(entry.mode && entry.parameter && entry.request_motion && update.event_object &&
            update.difference && update.origin && update.fire && update.actor_flag && update.complete && effects.complete()) { }
    StormboltLastSession(const StormboltLastSession&)=delete;
    StormboltLastSession& operator=(const StormboltLastSession&)=delete;
    bool install() { return complete_ && hook_.install(bindings_); }
    bool restore() { return hook_.restore(); }
};
} // namespace stormslinger
