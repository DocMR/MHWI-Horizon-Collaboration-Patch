#pragma once
#include "../../../runtime/src/stormbolt_loop_audio_dispatch.h"
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>
extern "C" void stormbolt_loop_update_span_entry();
namespace horizon_other {
// Single equipment combine-update call owner; choose this OR the shared full
// session for this site. Host supplies the SAME tracked loop/status stores.
template<std::size_t LoopCapacity, std::size_t StatusCapacity>
class ShieldLoopCall {
    stormslinger::StormboltLoopComponentDispatch<LoopCapacity,StatusCapacity> dispatch_;
    std::optional<stormslinger::StormboltOwnedReplayCallSite<7>> call_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    std::uintptr_t site_;
    bool attempted_ = false, prepared_ = false, published_ = false, activated_ = false;
    bool matching_component_;
public:
    ShieldLoopCall(const std::uint8_t* certified_image,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellStore<StatusCapacity>& statuses,
        const stormslinger::StormboltLoopComponentUpdateBindings& bindings)
        : dispatch_(loops, statuses, bindings),
          site_(certified_image ? reinterpret_cast<std::uintptr_t>(certified_image)+0x12EE81D : 0),
          matching_component_(certified_image && bindings.audio.component_vtable ==
              reinterpret_cast<std::uintptr_t>(certified_image)+0x324A830) {}
    ShieldLoopCall(const ShieldLoopCall&) = delete;
    ShieldLoopCall& operator=(const ShieldLoopCall&) = delete;
    // Host authenticates full image, material/audio APIs, resource lifetimes and
    // native scheduling. All operations require execution/writers quiescent.
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !matching_component_ || !transaction.can_add_hooks() ||
            stormbolt_loop_update_dispatch_target) return false;
        attempted_ = true;
        transaction_ = &transaction;
        if (!dispatch_.publish()) return false;
        published_ = true;
        call_.emplace(&transaction);
        if (!call_->prepare(reinterpret_cast<void*>(site_),
                reinterpret_cast<std::uintptr_t>(&stormbolt_loop_update_span_entry),
                {{0x80,0xBF,0xB8,0x09,0,0,0}})) return false;
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
    // Restore and drain native frames before shared lifecycle hook removal.
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
