#pragma once
#include "stormbolt_middle_entry.h"
#include "stormbolt_middle_update.h"
#include "stormbolt_special_to_normal_hook.h"

namespace stormslinger {
template<std::size_t Capacity,bool Primary=false>
class StormboltMiddleCore {
    StormboltActionFields<Capacity> fields_;
    StormboltMiddleEntry<Capacity> entry_;
    StormboltMiddleUpdate<Capacity> update_;
public:
    StormboltMiddleCore(StormboltInstanceStore<Capacity>& store,
        const StormboltMiddleEntryApi& entry,const StormboltMiddlePrepareApi& prepare,
        const StormboltMiddleFireApi& fire,const StormboltSpecialToNormalApi& action)
        : fields_(store),entry_(store,entry,&fields_),update_(store,prepare,fire,action,&fields_) { }
    auto enter(void* action) { return entry_.enter(action); }
    auto update(void* action) { return update_.update(action); }
};
// Owns only entry/update slots. Exit remains native. Caller must keep the session
// and borrowed dependencies alive until successful restore at a quiescent point.
template<std::size_t Capacity,bool Primary=false>
class StormboltMiddleSession {
    StormboltMiddleCore<Capacity,Primary> core_;
    StormboltSpecialToNormalHook<Capacity,StormboltMiddleCore<Capacity,Primary>,OwnedVtablePointer<>,false> hook_;
    const StormboltSpecialToNormalBindings bindings_;
public:
    StormboltMiddleSession(StormboltInstanceStore<Capacity>& store,
        const StormboltMiddleEntryApi& entry,const StormboltMiddlePrepareApi& prepare,
        const StormboltMiddleFireApi& fire,const StormboltSpecialToNormalApi& action,
        const StormboltSpecialToNormalBindings& bindings)
        : core_(store,entry,prepare,fire,action),hook_(store,core_),bindings_(bindings) { }
    StormboltMiddleSession(const StormboltMiddleSession&)=delete;
    StormboltMiddleSession& operator=(const StormboltMiddleSession&)=delete;
    bool install() { return hook_.install(bindings_); }
    bool restore() { return hook_.restore(); }
};
} // namespace stormslinger
