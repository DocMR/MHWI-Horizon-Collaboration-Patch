#pragma once
#include "../../../runtime/src/stormbolt_lifecycle_hooks.h"
#include "../../../runtime/src/stormbolt_damage_exception_store.h"
#include "../../../runtime/src/stormbolt_damage_exception_hooks.h"
#include "../../../runtime/src/stormbolt_power_cell_loop_state.h"
#include "../../../runtime/src/stormbolt_power_cell_sync_store.h"
namespace horizon_other {
bool verify_shield_auxiliary_lifecycle_owners(const std::uint8_t* image) noexcept;
enum class ShieldAuxPrepare { prepared, invalid_state, owner_mismatch, damage_failed, damage_events_failed, loop_failed, sync_failed };
// Borrow the unified host's stores. This queues eight hooks in its transaction;
// it does not begin/commit/stop that transaction or create a second store.
template<std::size_t Capacity, std::size_t LoopCapacity>
class ShieldAuxiliaryLifecycles {
    const std::uint8_t* image_;
    stormslinger::StormboltLifecycleHooks<Capacity,stormslinger::StormboltDamageExceptionState,
        stormslinger::StormboltDamageExceptionToken> damage_;
    stormslinger::StormboltDamageExceptionHooks<Capacity> damage_events_;
    stormslinger::StormboltLifecycleHooks<LoopCapacity,stormslinger::StormboltPowerCellLoopState,
        stormslinger::StormboltPowerCellLoopToken> loop_;
    stormslinger::StormboltLifecycleHooks<Capacity,stormslinger::StormboltPowerCellSyncEventState,
        stormslinger::StormboltPowerCellSyncToken> sync_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    bool attempted_ = false;
public:
    ShieldAuxiliaryLifecycles(const std::uint8_t* certified_image,
        stormslinger::StormboltDamageExceptionStore<Capacity>& damages,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& sync)
        : image_(certified_image), damage_(damages),
          damage_events_(damages, certified_image ? reinterpret_cast<std::uintptr_t>(certified_image)+0x3240FB0 : 0),
          loop_(loops), sync_(sync) {}
    ShieldAuxiliaryLifecycles(const ShieldAuxiliaryLifecycles&) = delete;
    ShieldAuxiliaryLifecycles& operator=(const ShieldAuxiliaryLifecycles&) = delete;
    // Host certifies the executable and holds target execution/writers quiescent.
    // Must be prepared and committed before the corresponding objects are born.
    ShieldAuxPrepare prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !image_ || !transaction.can_add_hooks()) return ShieldAuxPrepare::invalid_state;
        attempted_ = true;
        transaction_ = &transaction;
        if (!verify_shield_auxiliary_lifecycle_owners(image_)) return ShieldAuxPrepare::owner_mismatch;
        const auto base = reinterpret_cast<std::uintptr_t>(image_);
        if (damage_.add_constructor_to_transaction(transaction, reinterpret_cast<void*>(base+0x1216EF0),
                reinterpret_cast<void*>(base+0x1217340)) != MH_OK) return ShieldAuxPrepare::damage_failed;
        if (damage_events_.add_to_transaction(transaction, reinterpret_cast<void*>(base+0x121B8D0),
                reinterpret_cast<void*>(base+0x118EFE0)) != MH_OK) return ShieldAuxPrepare::damage_events_failed;
        if (loop_.add_constructor_to_transaction(transaction, reinterpret_cast<void*>(base+0x12EA460),
                reinterpret_cast<void*>(base+0x12EAA40)) != MH_OK) return ShieldAuxPrepare::loop_failed;
        if (sync_.add_constructor_to_transaction(transaction, reinterpret_cast<void*>(base+0x11927A0),
                reinterpret_cast<void*>(base+0x1193610)) != MH_OK) return ShieldAuxPrepare::sync_failed;
        return ShieldAuxPrepare::prepared;
    }
    // Restore/drain ALL growth/status/loop/sync CALLs first. Then the host stops
    // the entire transaction, drains lifecycle frames and invokes this method.
    // No hot unload or automatic destructor cleanup is provided.
    bool release_after_shared_stop() {
        if (!transaction_ || transaction_->has_library_state()) return false;
        if (sync_.owns_bridge() && !sync_.release_after_unhook()) return false;
        if (loop_.owns_bridge() && !loop_.release_after_unhook()) return false;
        if (damage_events_.owns_bridge() && !damage_events_.release_after_unhook()) return false;
        if (damage_.owns_bridge() && !damage_.release_after_unhook()) return false;
        return true;
    }
};
}
