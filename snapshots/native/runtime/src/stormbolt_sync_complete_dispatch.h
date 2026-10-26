#pragma once
#include "stormbolt_sync_complete_entry.h"
#include "stormbolt_power_cell_sync_frames.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
// The pack callback captures the frame first. Withdraw only after restoring
// both producer CALLs and draining outstanding native frames.
template<std::size_t FrameCapacity,std::size_t StoreCapacity>
class StormboltSyncCompleteDispatch {
    StormboltPowerCellSyncFrames<FrameCapacity>& frames_;
    StormboltPowerCellSyncStore<StoreCapacity>& events_;
    void (*previous_)(void*,std::uintptr_t)=nullptr;
    inline static StormboltCallbackPublication<StormboltSyncCompleteDispatch> publication_;
    static void dispatch(void* controller,std::uintptr_t stack) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (owner) (void)owner->frames_.complete(stack,
            reinterpret_cast<std::uintptr_t>(controller),owner->events_);
    }
public:
    StormboltSyncCompleteDispatch(StormboltPowerCellSyncFrames<FrameCapacity>& frames,
        StormboltPowerCellSyncStore<StoreCapacity>& events):frames_(frames),events_(events) {}
    bool publish() {
        if (!publication_.publish(this)) return false;
        previous_=stormbolt_sync_complete_dispatch_target;
        stormbolt_sync_complete_dispatch_target=&dispatch;return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || frames_.active() || stormbolt_sync_complete_dispatch_target!=&dispatch) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_sync_complete_dispatch_target=previous_;return true;
    }
};
} // namespace stormslinger
