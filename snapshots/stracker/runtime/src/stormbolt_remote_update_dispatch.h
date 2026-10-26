#pragma once
#include "stormbolt_remote_update_entry.h"
#include "stormbolt_power_cell_remote_native_adapter.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
using StormboltRemoteFlagDecoder=bool (*)(const StormboltPowerCellRemoteCache&,
    StormboltPowerCellRemoteFlags&,bool&);
struct StormboltRemoteUpdateBindings {
    StormboltLoopInputBindings inputs{};
    StormboltLoopAudioUpdateBindings audio{};
    void** manager_slot=nullptr;
    void* (*local_player)(void*)=nullptr;
    StormboltEquipmentEffectRequest equipment=nullptr;
    StormboltRemoteFlagDecoder decode=nullptr;
};
// Publish before CALL installation; withdraw after restoration and native
// frame drain. The decoder must implement the verified PC transport mapping.
template<std::size_t StatusCapacity,std::size_t LoopCapacity>
class StormboltRemoteUpdateDispatch {
    StormboltPowerCellStore<StatusCapacity>& statuses_;
    StormboltPowerCellLoopStore<LoopCapacity>& loops_;
    const StormboltRemoteUpdateBindings bindings_;
    void (*previous_)(void*)=nullptr;
    inline static StormboltCallbackPublication<StormboltRemoteUpdateDispatch> publication_;
    static void dispatch(void* component) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (!owner) return;
        StormboltPowerCellRemoteCache cache{};
        StormboltPowerCellRemoteFlags flags{};bool previous=false;
        if (!read_stormbolt_power_cell_remote_cache(component,cache) ||
            !owner->bindings_.decode(cache,flags,previous)) return;
        StormboltPowerCellRemoteDecision decision{};
        StormboltPowerCellPostEffectsResult effects{};
        (void)process_stormbolt_power_cell_remote_native(cache,flags,previous,
            owner->statuses_,owner->loops_,owner->bindings_.inputs,
            owner->bindings_.audio,owner->bindings_.manager_slot,
            owner->bindings_.local_player,owner->bindings_.equipment,decision,effects);
    }
public:
    StormboltRemoteUpdateDispatch(StormboltPowerCellStore<StatusCapacity>& statuses,
        StormboltPowerCellLoopStore<LoopCapacity>& loops,
        const StormboltRemoteUpdateBindings& bindings):statuses_(statuses),loops_(loops),bindings_(bindings) {}
    bool publish() {
        if (!bindings_.decode || !bindings_.manager_slot ||
            !bindings_.local_player || !bindings_.equipment || !publication_.publish(this)) return false;
        previous_=stormbolt_remote_update_dispatch_target;
        stormbolt_remote_update_dispatch_target=&dispatch;
        return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_remote_update_dispatch_target!=&dispatch) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_remote_update_dispatch_target=previous_;
        return true;
    }
};
} // namespace stormslinger
