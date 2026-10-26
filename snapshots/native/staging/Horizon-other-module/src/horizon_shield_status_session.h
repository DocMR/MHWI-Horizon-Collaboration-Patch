#pragma once
#include "horizon_shield_status_calls.h"
#include "../../../runtime/src/stormbolt_status_lifecycle_hooks.h"

namespace horizon_other {
bool verify_shield_status_lifecycle_owners(const std::uint8_t* image) noexcept;
enum class ShieldStatusPrepare { prepared, invalid_state, owner_mismatch, lifecycle_failed, calls_failed };
// Equipment-only composition, borrowing the unified host's ONE status store.
// No shooting/class48 integration; no automatic installation or self-owned store.
template<std::size_t Capacity>
class ShieldStatusSession {
    const std::uint8_t* image_;
    stormslinger::StormboltStatusLifecycleHooks<Capacity> lifecycle_;
    ShieldStatusCalls<Capacity> calls_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    bool attempted_ = false, matching_bindings_ = false;
public:
    ShieldStatusSession(const std::uint8_t* certified_image,
        stormslinger::StormboltPowerCellStore<Capacity>& shared,
        const stormslinger::StormboltStatusUpdateBindings& status,
        const stormslinger::StormboltTimerUpdateBindings& timer)
        : image_(certified_image),
          lifecycle_(shared, certified_image ? reinterpret_cast<std::uintptr_t>(certified_image)+0x322C268 : 0),
          calls_(shared, status, timer) {
        const auto base = reinterpret_cast<std::uintptr_t>(image_);
        matching_bindings_ = image_ && status.inputs.status_vtable == base+0x322C268 &&
            timer.inputs.status_vtable == base+0x322C268;
    }
    ShieldStatusSession(const ShieldStatusSession&) = delete;
    ShieldStatusSession& operator=(const ShieldStatusSession&) = delete;
    // Host certifies full executable, bindings/resources and keeps execution and
    // other writers quiescent through prepare, commit, activation and rollback.
    ShieldStatusPrepare prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !matching_bindings_ || !transaction.can_add_hooks())
            return ShieldStatusPrepare::invalid_state;
        attempted_ = true;
        transaction_ = &transaction;
        if (!verify_shield_status_lifecycle_owners(image_)) return ShieldStatusPrepare::owner_mismatch;
        const auto base = reinterpret_cast<std::uintptr_t>(image_);
        if (lifecycle_.add_to_transaction(transaction, reinterpret_cast<void*>(base+0x11B6450),
                reinterpret_cast<void*>(base+0x11C34A0), reinterpret_cast<void*>(base+0x11B68A0)) != MH_OK)
            return ShieldStatusPrepare::lifecycle_failed;
        if (!calls_.prepare_while_quiescent(transaction, image_)) return ShieldStatusPrepare::calls_failed;
        return ShieldStatusPrepare::prepared;
    }
    bool activate_after_shared_commit_while_quiescent() {
        return calls_.activate_after_shared_commit_while_quiescent();
    }
    // Always restore CALLs before the host stops shared lifecycle hooks/mapping.
    bool restore_updates_while_quiescent() { return calls_.restore_while_quiescent(); }
    bool restoration_pending() const noexcept { return calls_.restoration_pending(); }
    // Host stopped the ENTIRE shared transaction and drained native callbacks.
    // Even after this succeeds, DLL/relays/state remain resident for process life.
    bool release_after_shared_stop() {
        if (!transaction_ || transaction_->has_library_state() || calls_.restoration_pending()) return false;
        if (!calls_.release_after_shared_stop()) return false;
        return !lifecycle_.owns_bridge() || lifecycle_.release_after_unhook();
    }
};
}
