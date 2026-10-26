#pragma once
#include "stormbolt_growth_dispatch.h"
#include "stormbolt_growth_entry.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_owned_call_site.h"
#include <optional>
extern "C" void stormbolt_growth_negative_entry_span();
extern "C" void stormbolt_growth_post_effect_entry_span();
extern "C" void stormbolt_growth_remaining_entry_span();

namespace stormslinger {
// All operations require native execution quiescent; restore and drain native
// frames before restoring the shared lifecycle transaction.
template<std::size_t S,std::size_t L>
class StormboltGrowthSession {
    const std::array<std::uintptr_t,3> sites_;
    StormboltGrowthDispatch<S,S,L,S> dispatch_;
    HookTransaction local_;
    HookTransaction* transaction_=nullptr;
    std::optional<StormboltOwnedReplayCallSite<8>> negative_;
    std::optional<StormboltOwnedReplayCallSite<9>> post_;
    std::optional<StormboltOwnedReplayCallSite<12>> remaining_;
    bool prepared_=false,published_=false;
public:
    StormboltGrowthSession(const std::array<std::uintptr_t,3>& sites,
        StormboltPowerCellStore<S>& statuses,StormboltDamageExceptionStore<S>& damages,
        StormboltPowerCellLoopStore<L>& loops,StormboltPowerCellSyncStore<S>& sync,
        const StormboltPowerCellCompleteGrowthBindings& bindings)
        :sites_(sites),dispatch_(statuses,damages,loops,sync,bindings) {}
    bool active() const noexcept {
        return prepared_ || published_ || (negative_ && negative_->active()) || (post_ && post_->active()) || (remaining_ && remaining_->active());
    }
    bool prepare(HookTransaction* shared=nullptr) {
        if(active() || transaction_ || !sites_[0] || !sites_[1] || !sites_[2]) return false;
        transaction_=shared?shared:&local_;
        if(!shared) {
            if(local_.begin()!=MH_OK) {transaction_=nullptr;return false;}
            local_.retain_failed_adds_for_coordinator();
        }
        if(!dispatch_.publish()) return false;
        published_=true;
        negative_.emplace(transaction_);post_.emplace(transaction_);remaining_.emplace(transaction_);
        if(!negative_->prepare(reinterpret_cast<void*>(sites_[0]),reinterpret_cast<std::uintptr_t>(&stormbolt_growth_negative_entry_span),
            std::array<unsigned char,8>{0x85,0xF6,0x0F,0x88,0x96,0,0,0})) return false;
        if(!post_->prepare(reinterpret_cast<void*>(sites_[1]),reinterpret_cast<std::uintptr_t>(&stormbolt_growth_post_effect_entry_span),
            std::array<unsigned char,9>{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76})) return false;
        if(!remaining_->prepare(reinterpret_cast<void*>(sites_[2]),reinterpret_cast<std::uintptr_t>(&stormbolt_growth_remaining_entry_span),
            std::array<unsigned char,12>{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0})) return false;
        prepared_=true;return true;
    }
    bool activate() {
        if(!prepared_ || !transaction_) return false;
        if(transaction_==&local_ && local_.commit_pending_without_rollback()!=MH_OK) return false;
        return negative_->activate() && post_->activate() && remaining_->activate();
    }
    bool restore_patches() {
        if((remaining_ && !remaining_->restore()) || (post_ && !post_->restore()) || (negative_ && !negative_->restore())) return false;
        return true;
    }
    bool withdraw_after_quiescence() {
        if((remaining_ && remaining_->active()) || (post_ && post_->active()) || (negative_ && negative_->active())) return false;
        if(published_) {if(!dispatch_.withdraw()) return false;published_=false;}
        prepared_=false;return true;
    }
    bool release_after_shared_stop() {
        if(active()) return false;
        if(!transaction_) return true;
        if(transaction_->has_library_state()) return false;
        return (!negative_ || negative_->release_after_quiescence()) &&
            (!post_ || post_->release_after_quiescence()) && (!remaining_ || remaining_->release_after_quiescence());
    }
    bool restore() {
        if(!restore_patches() || !withdraw_after_quiescence()) return false;
        if(transaction_==&local_) return local_.stop()==MH_OK && release_after_shared_stop();
        return true;
    }
};
} // namespace stormslinger
