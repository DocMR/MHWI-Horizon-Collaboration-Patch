#pragma once
#include "stormbolt_status_update_entry.h"
#include "stormbolt_power_cell_event_eligibility_adapter.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
struct StormboltStatusUpdateBindings {
    StormboltLoopInputBindings inputs{};
    StormboltSkillValueQuery query=nullptr;
    void (*actor_flag)(void*,std::uint32_t)=nullptr;
    std::uintptr_t manager_slot=0;
};
// Publish at a quiescent boundary before installing CALL; withdraw only after
// CALL restoration and completion of in-flight native frames.
template<std::size_t Capacity>
class StormboltStatusUpdateDispatch {
    StormboltPowerCellStore<Capacity>& store_;
    const StormboltStatusUpdateBindings bindings_;
    void (*previous_)(void*)=nullptr;
    std::uintptr_t previous_slot_=0;
    inline static StormboltCallbackPublication<StormboltStatusUpdateDispatch> publication_;
    static void dispatch(void* status) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (owner) (void)update_stormbolt_power_cell_event_eligibility_from_status(
            status,owner->store_,owner->bindings_.inputs,
            owner->bindings_.query,owner->bindings_.actor_flag);
    }
public:
    StormboltStatusUpdateDispatch(StormboltPowerCellStore<Capacity>& store,
        const StormboltStatusUpdateBindings& bindings):store_(store),bindings_(bindings) {}
    bool publish() {
        if (!bindings_.manager_slot || !bindings_.query || !bindings_.actor_flag || !publication_.publish(this)) return false;
        previous_=stormbolt_status_update_dispatch_target;
        previous_slot_=stormbolt_status_update_manager_slot;
        stormbolt_status_update_manager_slot=bindings_.manager_slot;
        stormbolt_status_update_dispatch_target=&dispatch;
        return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_status_update_dispatch_target!=&dispatch ||
            stormbolt_status_update_manager_slot!=bindings_.manager_slot) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_status_update_dispatch_target=previous_;
        stormbolt_status_update_manager_slot=previous_slot_;
        return true;
    }
};
} // namespace stormslinger
