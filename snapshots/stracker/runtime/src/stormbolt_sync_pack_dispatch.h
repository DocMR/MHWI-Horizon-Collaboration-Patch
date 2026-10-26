#pragma once
#include "stormbolt_sync_pack_entry.h"
#include "stormbolt_power_cell_sync_frames.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
using StormboltSyncEncoder=bool (*)(std::uint32_t,StormboltPowerCellRemoteFlags,std::uint32_t&);
struct StormboltSyncPackBindings {
    std::uintptr_t master_vtable=0;
    StormboltLoopInputBindings inputs{};
    StormboltSkillValueQuery query=nullptr;
    StormboltSyncEncoder encode=nullptr;
};
template<std::size_t FrameCapacity,std::size_t ControllerCapacity,std::size_t StatusCapacity>
class StormboltSyncPackDispatch {
    StormboltPowerCellSyncFrames<FrameCapacity>& frames_;
    StormboltPowerCellSyncStore<ControllerCapacity>& events_;
    StormboltPowerCellStore<StatusCapacity>& statuses_;
    const StormboltSyncPackBindings bindings_;
    std::uint32_t (*previous_)(void*,std::uintptr_t,std::uint32_t)=nullptr;
    inline static StormboltCallbackPublication<StormboltSyncPackDispatch> publication_;
    static std::uint32_t dispatch(void* controller,std::uintptr_t stack,std::uint32_t native_word) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (!owner) return native_word;
        StormboltPowerCellSyncSnapshot snapshot{};
        std::uint32_t encoded=native_word;
        if (!read_stormbolt_power_cell_sync_snapshot(controller,owner->bindings_.master_vtable,
            owner->events_,owner->statuses_,owner->bindings_.inputs,owner->bindings_.query,snapshot) ||
            !owner->bindings_.encode(native_word,snapshot.flags,encoded) ||
            !owner->frames_.begin(stack,snapshot)) return native_word;
        return encoded;
    }
public:
    StormboltSyncPackDispatch(StormboltPowerCellSyncFrames<FrameCapacity>& frames,
        StormboltPowerCellSyncStore<ControllerCapacity>& events,
        StormboltPowerCellStore<StatusCapacity>& statuses,const StormboltSyncPackBindings& bindings)
        :frames_(frames),events_(events),statuses_(statuses),bindings_(bindings) {}
    bool publish() {
        if (!bindings_.master_vtable || !bindings_.query || !bindings_.encode || !publication_.publish(this)) return false;
        previous_=stormbolt_sync_pack_dispatch_target;
        stormbolt_sync_pack_dispatch_target=&dispatch;return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_sync_pack_dispatch_target!=&dispatch) return false;
        if(!publication_.withdraw(this,+[](StormboltSyncPackDispatch* owner){return !owner->frames_.active();})) return false;
        stormbolt_sync_pack_dispatch_target=previous_;return true;
    }
};
} // namespace stormslinger
