#pragma once
#include "stormbolt_sync_pack_dispatch.h"
#include "stormbolt_sync_complete_dispatch.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_owned_call_site.h"
#include <optional>
extern "C" void stormbolt_sync_pack_span_entry();
extern "C" void stormbolt_sync_complete_span_entry();

namespace stormslinger {
// Caller keeps native execution quiescent across preparation, activation,
// restore and frame drain. This session owns only the two producer CALLs.
template<std::size_t Capacity>
class StormboltSyncProducerSession {
    const std::uintptr_t pack_site_,complete_site_;
    StormboltPowerCellSyncFrames<Capacity> frames_;
    StormboltSyncPackDispatch<Capacity,Capacity,Capacity> pack_dispatch_;
    StormboltSyncCompleteDispatch<Capacity,Capacity> complete_dispatch_;
    HookTransaction local_;
    HookTransaction* transaction_=nullptr;
    std::optional<StormboltOwnedReplayCallSite<6>> pack_call_;
    std::optional<StormboltOwnedReplayCallSite<8>> complete_call_;
    bool pack_published_=false,complete_published_=false,prepared_=false;
public:
    StormboltSyncProducerSession(std::uintptr_t pack_site,std::uintptr_t complete_site,
        StormboltPowerCellSyncStore<Capacity>& events,StormboltPowerCellStore<Capacity>& statuses,
        const StormboltSyncPackBindings& bindings)
        :pack_site_(pack_site),complete_site_(complete_site),
         pack_dispatch_(frames_,events,statuses,bindings),complete_dispatch_(frames_,events) {}
    bool in_flight() const noexcept { return frames_.active(); }
    bool active() const noexcept {
        return prepared_ || pack_published_ || complete_published_ ||
            (pack_call_ && pack_call_->active()) || (complete_call_ && complete_call_->active()) || frames_.active();
    }
    bool prepare(HookTransaction* shared=nullptr) {
        if(active() || transaction_ || !pack_site_ || !complete_site_) return false;
        transaction_=shared?shared:&local_;
        if(!shared) {
            if(local_.begin()!=MH_OK) {transaction_=nullptr;return false;}
            local_.retain_failed_adds_for_coordinator();
        }
        if (!complete_dispatch_.publish()) return false;
        complete_published_=true;
        if (!pack_dispatch_.publish()) return false;
        pack_published_=true;
        complete_call_.emplace(transaction_);pack_call_.emplace(transaction_);
        if(!complete_call_->prepare(reinterpret_cast<void*>(complete_site_),
            reinterpret_cast<std::uintptr_t>(&stormbolt_sync_complete_span_entry),
            std::array<unsigned char,8>{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0})) return false;
        if(!pack_call_->prepare(reinterpret_cast<void*>(pack_site_),
            reinterpret_cast<std::uintptr_t>(&stormbolt_sync_pack_span_entry),
            std::array<unsigned char,6>{0x89,0x7D,0xA7,0x4C,0x2B,0xC0})) return false;
        prepared_=true;return true;
    }
    bool activate() {
        if(!prepared_ || !transaction_) return false;
        if(transaction_==&local_ && local_.commit_pending_without_rollback()!=MH_OK) return false;
        return complete_call_->activate() && pack_call_->activate();
    }
    bool restore_patches() {
        if ((pack_call_ && !pack_call_->restore())) return false;
        // A packed native frame still needs its completion CALL. Restore that
        // site only after completion; retain everything for a later retry.
        if(frames_.active() || (complete_call_ && !complete_call_->restore())) return false;
        return true;
    }
    bool withdraw_after_quiescence() {
        if((pack_call_ && pack_call_->active()) || (complete_call_ && complete_call_->active())) return false;
        if (pack_published_) {if (!pack_dispatch_.withdraw()) return false;pack_published_=false;}
        if (complete_published_) {if (!complete_dispatch_.withdraw()) return false;complete_published_=false;}
        prepared_=false;return true;
    }
    bool release_after_shared_stop() {
        if(active()) return false;
        if(!transaction_) return true;
        if(transaction_->has_library_state()) return false;
        return (!pack_call_ || pack_call_->release_after_quiescence()) &&
            (!complete_call_ || complete_call_->release_after_quiescence());
    }
    bool restore() {
        if(!restore_patches() || !withdraw_after_quiescence()) return false;
        if(transaction_==&local_) return local_.stop()==MH_OK && release_after_shared_stop();
        return true;
    }
};
} // namespace stormslinger
