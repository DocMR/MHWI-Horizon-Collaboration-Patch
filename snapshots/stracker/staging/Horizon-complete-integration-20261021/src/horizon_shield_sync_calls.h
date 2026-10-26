#pragma once
#include "../../../runtime/src/stormbolt_sync_producer_session.h"
namespace horizon_other {
// Reuse the shared producer implementation and its ONE frame set. Borrow the
// same transaction/status/event stores; never initialize a private hook library.
template<std::size_t Capacity>
class ShieldSyncCalls {
    stormslinger::StormboltSyncProducerSession<Capacity> producer_;
    stormslinger::HookTransaction* transaction_=nullptr;
    bool attempted_=false,prepared_=false,activated_=false;
public:
    ShieldSyncCalls(const std::uint8_t* image,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& events,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        const stormslinger::StormboltSyncPackBindings& bindings)
        :producer_(image?reinterpret_cast<std::uintptr_t>(image)+0x1199E99:0,
                   image?reinterpret_cast<std::uintptr_t>(image)+0x1199F4B:0,
                   events,statuses,bindings) {}
    ShieldSyncCalls(const ShieldSyncCalls&)=delete;
    ShieldSyncCalls& operator=(const ShieldSyncCalls&)=delete;
    bool in_flight() const noexcept { return producer_.in_flight(); }
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !transaction.can_add_hooks() ||
            stormbolt_sync_pack_dispatch_target || stormbolt_sync_complete_dispatch_target) return false;
        attempted_=true;transaction_=&transaction;
        prepared_=producer_.prepare(&transaction);
        return prepared_;
    }
    bool activate_after_shared_commit_while_quiescent() {
        if (activated_ || !prepared_ || !transaction_ || !transaction_->last.active) return false;
        activated_=producer_.activate();return activated_;
    }
    // Host first prevents new producers and drains native frames. The shared
    // implementation disables pack before complete and retains pending frames.
    bool restore_while_quiescent() {
        if (!producer_.restore()) return false;
        prepared_=false;activated_=false;return true;
    }
    bool restoration_pending() const noexcept { return producer_.active(); }
    bool release_after_shared_stop() { return producer_.release_after_shared_stop(); }
};
}
