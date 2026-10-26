#pragma once
#include "../../../runtime/src/stormbolt_status_update_dispatch.h"
#include "../../../runtime/src/stormbolt_timer_update_dispatch.h"
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>

extern "C" void horizon_status_update_span_entry();
extern "C" void horizon_timer_update_span_entry();

namespace horizon_other {
// Optional equipment-only owner for the TWO shared status callsites. Use either
// this owner OR the full shared session, never both. Host owns authenticated
// lifecycle hooks/store, native bindings, early bootstrap and thread quiescence.
// This object and its DLL/relays remain resident for process lifetime.
template<std::size_t Capacity>
class ShieldStatusCalls {
    stormslinger::StormboltStatusUpdateDispatch<Capacity> status_;
    stormslinger::StormboltTimerUpdateDispatch<Capacity> timer_;
    std::optional<stormslinger::StormboltOwnedReplayCallSite<7>> status_call_, timer_call_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    std::uintptr_t status_site_ = 0, timer_site_ = 0;
    bool attempted_ = false, prepared_ = false;
    bool status_published_ = false, timer_published_ = false;
public:
    ShieldStatusCalls(stormslinger::StormboltPowerCellStore<Capacity>& shared,
        const stormslinger::StormboltStatusUpdateBindings& status,
        const stormslinger::StormboltTimerUpdateBindings& timer)
        : status_(shared, status), timer_(shared, timer) {}
    ShieldStatusCalls(const ShieldStatusCalls&) = delete;
    ShieldStatusCalls& operator=(const ShieldStatusCalls&) = delete;
    // Host has certified the original executable and all APIs/types/resources.
    // No calls may execute through these sites throughout prepare/commit/restore.
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction,
                                const std::uint8_t* certified_image) {
        if (attempted_ || !certified_image || !transaction.can_add_hooks() ||
            stormbolt_status_update_dispatch_target || stormbolt_status_update_manager_slot ||
            stormbolt_timer_update_dispatch_target || stormbolt_timer_update_native_query) return false;
        attempted_ = true;
        transaction_ = &transaction;
        const auto base = reinterpret_cast<std::uintptr_t>(certified_image);
        status_site_ = base + 0x11C6CE3;
        timer_site_ = base + 0x11C7F18;
        if (!status_.publish()) return false;
        status_published_ = true;
        if (!timer_.publish()) return false;
        timer_published_ = true;
        status_call_.emplace(&transaction);
        timer_call_.emplace(&transaction);
        constexpr std::array<unsigned char,7> status_bytes{{0x48,0x8B,0x0D,0xB6,0x7F,0xE4,0x03}};
        constexpr std::array<unsigned char,7> timer_bytes{{0xE8,0x83,0xA9,0xD9,0,0x84,0xC0}};
        if (!status_call_->prepare(reinterpret_cast<void*>(status_site_),
                reinterpret_cast<std::uintptr_t>(&horizon_status_update_span_entry),status_bytes) ||
            !timer_call_->prepare(reinterpret_cast<void*>(timer_site_),
                reinterpret_cast<std::uintptr_t>(&horizon_timer_update_span_entry),timer_bytes)) return false;
        prepared_ = true;
        return true;
    }
    bool activate_after_shared_commit_while_quiescent() {
        if (!prepared_ || !transaction_ || !transaction_->has_library_state() ||
            !transaction_->last.active || !status_call_ || !timer_call_) return false;
        // Root committed both replay CALLs along with every other queued hook.
        // Component activation checks publication; it never commits the queue.
        return status_call_->activate() && timer_call_->activate();
    }
    bool restore_while_quiescent() {
        // Restore both native sites before withdrawing any published callback.
        if ((timer_call_ && !timer_call_->restore()) ||
            (status_call_ && !status_call_->restore())) return false;
        if (timer_published_) {
            if (!timer_.withdraw()) return false;
            timer_published_ = false;
        }
        if (status_published_) {
            if (!status_.withdraw()) return false;
            status_published_ = false;
        }
        prepared_ = false;
        return true;
    }
    bool restoration_pending() const noexcept {
        return (status_call_ && status_call_->active()) || (timer_call_ && timer_call_->active()) ||
            status_published_ || timer_published_;
    }
    bool release_after_shared_stop() {
        if (!attempted_) return true; // Lifecycle preparation may fail before CALL preparation.
        if (!transaction_ || transaction_->has_library_state() || restoration_pending()) return false;
        return (!timer_call_ || timer_call_->release_after_quiescence()) &&
               (!status_call_ || status_call_->release_after_quiescence());
    }
};
}
