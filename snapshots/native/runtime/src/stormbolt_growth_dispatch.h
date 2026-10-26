#pragma once
#include "stormbolt_power_cell_growth_complete_adapter.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger {
using StormboltGrowthCallback=void (*)(void*,const void*,const void*);
extern "C" { inline StormboltGrowthCallback stormbolt_growth_dispatch_target=nullptr; }
// Caller publishes before installing native sites, restores sites and drains
// native frames before withdrawal. All stores are the shared session stores.
template<std::size_t S,std::size_t D,std::size_t L,std::size_t N>
class StormboltGrowthDispatch {
    StormboltPowerCellStore<S>& statuses_;
    StormboltDamageExceptionStore<D>& damages_;
    StormboltPowerCellLoopStore<L>& loops_;
    StormboltPowerCellSyncStore<N>& sync_;
    const StormboltPowerCellCompleteGrowthBindings bindings_;
    StormboltGrowthCallback previous_=nullptr;
    inline static StormboltCallbackPublication<StormboltGrowthDispatch> publication_;
    static void dispatch(void* component,const void* event,const void* result) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (!owner) return;
        StormboltPowerCellGrowthResult growth{};
        StormboltPowerCellPostEffectsResult effects{};
        (void)process_stormbolt_power_cell_complete_growth_event(component,event,result,
            owner->damages_,owner->statuses_,owner->loops_,owner->sync_,owner->bindings_,growth,effects);
    }
public:
    StormboltGrowthDispatch(StormboltPowerCellStore<S>& statuses,StormboltDamageExceptionStore<D>& damages,
        StormboltPowerCellLoopStore<L>& loops,StormboltPowerCellSyncStore<N>& sync,
        const StormboltPowerCellCompleteGrowthBindings& bindings)
        :statuses_(statuses),damages_(damages),loops_(loops),sync_(sync),bindings_(bindings) {}
    bool publish() {
        const auto& g=bindings_.growth;
        if (!bindings_.master_vtable || !bindings_.equipment || !g.damage_vtable ||
            !g.loop.query || !g.manager_slot || !g.local_player || !g.prompt || !publication_.publish(this)) return false;
        previous_=stormbolt_growth_dispatch_target;
        stormbolt_growth_dispatch_target=&dispatch;return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_growth_dispatch_target!=&dispatch) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_growth_dispatch_target=previous_;return true;
    }
};
} // namespace stormslinger
