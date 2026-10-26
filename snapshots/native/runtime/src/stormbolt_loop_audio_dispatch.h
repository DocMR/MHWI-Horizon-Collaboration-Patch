#pragma once
#include "stormbolt_loop_component_update.h"
#include "stormbolt_callback_publication.h"

namespace stormslinger { using StormboltLoopUpdateDispatch=void (*)(void*); }
extern "C" {
extern stormslinger::StormboltLoopUpdateDispatch stormbolt_loop_update_dispatch_target;
void stormbolt_loop_update_entry();
}
namespace stormslinger {
// Publish/withdraw only at quiescent boundaries; withdraw after restoring the
// CALL site. Audio compatibility bindings or combined component bindings
// select the respective adapter; publishing does not install game hooks.
template<std::size_t LoopCapacity,std::size_t StatusCapacity,class Bindings=StormboltLoopAudioUpdateBindings>
class StormboltLoopAudioDispatch {
    StormboltPowerCellLoopStore<LoopCapacity>& loops_;
    StormboltPowerCellStore<StatusCapacity>& statuses_;
    const Bindings bindings_;
    StormboltLoopUpdateDispatch previous_=nullptr;
    inline static StormboltCallbackPublication<StormboltLoopAudioDispatch> publication_;
    static void dispatch(void* component) {
        auto lease=publication_.acquire();auto* owner=lease.owner;
        if (owner) (void)update_stormbolt_loop_from_component(
            component,owner->loops_,owner->statuses_,owner->bindings_);
    }
public:
    StormboltLoopAudioDispatch(StormboltPowerCellLoopStore<LoopCapacity>& loops,
        StormboltPowerCellStore<StatusCapacity>& statuses,const Bindings& bindings)
        :loops_(loops),statuses_(statuses),bindings_(bindings) {}
    bool publish() {
        if (!publication_.publish(this)) return false;
        previous_=stormbolt_loop_update_dispatch_target;
        stormbolt_loop_update_dispatch_target=&dispatch;
        return true;
    }
    bool withdraw() {
        if (!publication_.owns(this) || stormbolt_loop_update_dispatch_target!=&dispatch) return false;
        if(!publication_.withdraw(this)) return false;
        stormbolt_loop_update_dispatch_target=previous_;
        return true;
    }
};
template<std::size_t L,std::size_t S>
using StormboltLoopComponentDispatch=StormboltLoopAudioDispatch<L,S,StormboltLoopComponentUpdateBindings>;
} // namespace stormslinger
