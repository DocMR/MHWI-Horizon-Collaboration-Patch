#pragma once
#include "stormbolt_timer_update_entry.h"
#include "stormbolt_power_cell_timer_adapter.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
struct StormboltTimerUpdateBindings {
    StormboltLoopInputBindings inputs{};
    StormboltSkillValueQuery query=nullptr;
    StormboltSkillPrompt prompt=nullptr;
    bool (*native_query)(void*)=nullptr;
};
// Publish before installing CALL at a quiescent boundary. Restore CALL and
// drain native frames before withdrawing either target.
template<std::size_t Capacity>
class StormboltTimerUpdateDispatch {
    StormboltPowerCellStore<Capacity>& store_;
    const StormboltTimerUpdateBindings bindings_;
    void (*previous_)(void*,float)=nullptr;
    bool (*previous_query_)(void*)=nullptr;
    inline static StormboltCallbackPublication<StormboltTimerUpdateDispatch> publication_;
    static void dispatch(void* status,float step) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (owner) (void)advance_stormbolt_power_cell_timer_from_status(
            status,step,owner->store_,owner->bindings_.inputs,
            owner->bindings_.query,owner->bindings_.prompt);
    }
public:
    StormboltTimerUpdateDispatch(StormboltPowerCellStore<Capacity>& store,
        const StormboltTimerUpdateBindings& bindings):store_(store),bindings_(bindings) {}
    bool publish() {
        if (!bindings_.query || !bindings_.prompt || !bindings_.native_query || !publication_.publish(this)) return false;
        previous_=stormbolt_timer_update_dispatch_target;
        previous_query_=stormbolt_timer_update_native_query;
        stormbolt_timer_update_native_query=bindings_.native_query;
        stormbolt_timer_update_dispatch_target=&dispatch;
        return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_timer_update_dispatch_target!=&dispatch ||
            stormbolt_timer_update_native_query!=bindings_.native_query) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_timer_update_dispatch_target=previous_;
        stormbolt_timer_update_native_query=previous_query_;
        return true;
    }
};
} // namespace stormslinger
