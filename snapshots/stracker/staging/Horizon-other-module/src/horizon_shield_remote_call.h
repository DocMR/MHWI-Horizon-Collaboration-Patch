#pragma once
#include "../../../runtime/src/stormbolt_remote_update_dispatch.h"
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>
// Provided by the existing shared remote-update MASM object, linked once.
extern "C" void stormbolt_remote_update_span_entry();
namespace horizon_other {
// Equipment remote-state application only; one owner of the shared history-copy
// boundary. Host supplies the SAME tracked stores and matching producer codec.
template<std::size_t StatusCapacity, std::size_t LoopCapacity>
class ShieldRemoteCall {
    stormslinger::StormboltRemoteUpdateDispatch<StatusCapacity,LoopCapacity> dispatch_;
    std::optional<stormslinger::StormboltOwnedReplayCallSite<6>> call_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    std::uintptr_t site_;
    bool attempted_ = false, prepared_ = false, published_ = false, activated_ = false;
public:
    ShieldRemoteCall(const std::uint8_t* certified_image,
        stormslinger::StormboltPowerCellStore<StatusCapacity>& statuses,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        const stormslinger::StormboltRemoteUpdateBindings& bindings)
        : dispatch_(statuses, loops, bindings),
          site_(certified_image ? reinterpret_cast<std::uintptr_t>(certified_image)+0x11B034A : 0) {}
    ShieldRemoteCall(const ShieldRemoteCall&) = delete;
    ShieldRemoteCall& operator=(const ShieldRemoteCall&) = delete;
    // Host authenticates image, native cache/codec/API/resource identities and
    // guarantees native execution and other code writers remain quiescent.
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !site_ || !transaction.can_add_hooks() ||
            stormbolt_remote_update_dispatch_target) return false;
        attempted_ = true;
        transaction_ = &transaction;
        if (!dispatch_.publish()) return false;
        published_ = true;
        call_.emplace(&transaction);
        if (!call_->prepare(reinterpret_cast<void*>(site_),
                reinterpret_cast<std::uintptr_t>(&stormbolt_remote_update_span_entry),
                {{0x8B,0x87,0x98,0x59,0,0}})) return false;
        prepared_ = true;
        return true;
    }
    bool activate_after_shared_commit_while_quiescent() {
        if (activated_ || !prepared_ || !transaction_ || !transaction_->has_library_state() ||
            !transaction_->last.active) return false;
        if (!call_ || !call_->activate()) {
            (void)restore_while_quiescent();
            return false;
        }
        activated_ = true;
        return true;
    }
    // Host drains native callbacks before restoration and shared lifecycle stop.
    bool restore_while_quiescent() {
        if (call_ && !call_->restore()) return false;
        if (published_) {
            if (!dispatch_.withdraw()) return false;
            published_ = false;
        }
        prepared_ = false;
        activated_ = false;
        return true;
    }
    bool restoration_pending() const noexcept { return (call_ && call_->active()) || published_; }
    bool release_after_shared_stop() {
        if (!attempted_) return true;
        if (!transaction_ || transaction_->has_library_state() || restoration_pending()) return false;
        return !call_ || call_->release_after_quiescence();
    }
};
}
