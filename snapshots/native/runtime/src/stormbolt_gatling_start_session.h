#pragma once
#include "stormbolt_gatling_start_entry.h"
#include "stormbolt_gatling_start_update.h"
#include "stormbolt_special_to_normal_hook.h"

namespace stormslinger {
template<std::size_t Capacity>
class StormboltGatlingStartCore {
    StormboltGatlingStartEntry<Capacity> entry_;
    StormboltGatlingStartUpdate<Capacity> update_;
public:
    StormboltGatlingStartCore(StormboltInstanceStore<Capacity>& store, const StormboltGatlingEntryApi& api)
        : entry_(store,api), update_(store,api.animation) { }
    auto enter(void* action) { return entry_.enter(action); }
    auto update(void* action) { return update_.update(action); }
};
// Shares authenticated target selection/ownership with the transition dispatcher.
// Only entry30/update38 are installed; no exit callback or exit-core exists.
template<std::size_t Capacity>
class StormboltGatlingStartSession {
    StormboltGatlingStartCore<Capacity> core_;
    StormboltSpecialToNormalHook<Capacity, StormboltGatlingStartCore<Capacity>, OwnedVtablePointer<>, false> hook_;
    const StormboltSpecialToNormalBindings bindings_;
public:
    StormboltGatlingStartSession(StormboltInstanceStore<Capacity>& store, const StormboltGatlingEntryApi& api,
        const StormboltSpecialToNormalBindings& bindings) : core_(store,api), hook_(store,core_), bindings_(bindings) { }
    StormboltGatlingStartSession(const StormboltGatlingStartSession&) = delete;
    StormboltGatlingStartSession& operator=(const StormboltGatlingStartSession&) = delete;
    bool install() { return hook_.install(bindings_); }
    bool restore() { return hook_.restore(); }
};
} // namespace stormslinger
