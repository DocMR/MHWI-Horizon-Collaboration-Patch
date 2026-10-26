#pragma once
#include "../../../runtime/src/stormbolt_loop_update_inputs.h"
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>
extern "C" {
extern bool (*horizon_shield_reaction_dispatch)(void*);
void horizon_shield_reaction_entry();
void horizon_shield_reaction_span_entry();
}
namespace horizon_other {
bool verify_shield_reaction_owner(const std::uint8_t* image) noexcept;
template<std::size_t Capacity>
class ShieldReactionCall {
    const std::uint8_t* image_;
    stormslinger::StormboltPowerCellStore<Capacity>& shared_;
    stormslinger::StormboltLoopInputBindings inputs_;
    stormslinger::StormboltSkillValueQuery query_;
    std::optional<stormslinger::StormboltOwnedReplayCallSite<6>> call_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    bool attempted_ = false, published_ = false, activated_ = false;
    inline static ShieldReactionCall* active_ = nullptr;
    static bool dispatch(void* status) {
        if (!active_ || !status) return false;
        auto& self = *active_;
        std::uintptr_t vtable=0,owner=0,owner_status=0;
        std::uint32_t sign_flags=0,flags=0;
        const auto address=reinterpret_cast<std::uintptr_t>(status);
        stormslinger::StormboltPowerCellToken token{};
        if (!self.shared_.token_for_live_address(address,token) ||
            !stormslinger::read_stormbolt_field(address,0,vtable) || vtable!=self.inputs_.status_vtable ||
            !stormslinger::read_stormbolt_field(address,8,owner) || !owner ||
            !stormslinger::read_stormbolt_field(owner,0x7D20,owner_status) || owner_status!=address ||
            !stormslinger::read_stormbolt_field(owner,0x8950,sign_flags) || !(sign_flags & 0x80000000u) ||
            !stormslinger::read_stormbolt_field(owner,0x8948,flags) || (flags & 0x00800000u)) return false;
        stormslinger::StormboltPowerCellLoopUpdateInputs input{};
        return stormslinger::read_stormbolt_loop_update_inputs(reinterpret_cast<void*>(owner),
            self.shared_,self.inputs_,self.query_,input) && input.owner_flag &&
            input.resource_index_present && input.resource_count>0 && input.count!=input.threshold;
    }

public:
    ShieldReactionCall(const std::uint8_t* certified_image,
        stormslinger::StormboltPowerCellStore<Capacity>& shared,
        const stormslinger::StormboltLoopInputBindings& inputs, stormslinger::StormboltSkillValueQuery query)
        : image_(certified_image),shared_(shared),inputs_(inputs),query_(query) {}
    ShieldReactionCall(const ShieldReactionCall&) = delete;
    ShieldReactionCall& operator=(const ShieldReactionCall&) = delete;
    // Host certifies the full image, borrowed resource/API lifetime and true
    // execution/writer quiescence, as for all other shared equipment callsites.
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !image_ || !query_ || !transaction.can_add_hooks() ||
            active_ || horizon_shield_reaction_dispatch) return false;
        attempted_=true;
        transaction_=&transaction;
        const auto base=reinterpret_cast<std::uintptr_t>(image_);
        if (inputs_.status_vtable!=base+0x322C268 || inputs_.skill_vtable!=base+0x324B1F0 ||
            !inputs_.resource_slot || !inputs_.resource_vtable || !verify_shield_reaction_owner(image_)) return false;
        active_=this;
        horizon_shield_reaction_dispatch=&dispatch;
        published_=true;
        call_.emplace(&transaction);
        return call_->prepare(const_cast<std::uint8_t*>(image_)+0x1217B3E,
            reinterpret_cast<std::uintptr_t>(&horizon_shield_reaction_span_entry),
            std::array<unsigned char,6>{0x45,0x32,0xFF,0x8B,0x5B,0x10});
    }
    bool activate_after_shared_commit_while_quiescent() {
        if (activated_ || !published_ || !transaction_ || !transaction_->has_library_state() ||
            !transaction_->last.active) return false;
        if (!call_ || !call_->activate()) {
            (void)restore_while_quiescent();return false;
        }
        activated_=true;return true;
    }
    bool restore_while_quiescent() {
        if (call_ && !call_->restore()) return false;
        if (published_) {
            if (active_!=this || horizon_shield_reaction_dispatch!=&dispatch) return false;
            horizon_shield_reaction_dispatch=nullptr;
            active_=nullptr;published_=false;
        }
        activated_=false;return true;
    }
    bool restoration_pending() const noexcept {return (call_ && call_->active()) || published_;}
    bool release_after_shared_stop() {
        if (!attempted_) return true;
        if (!transaction_ || transaction_->has_library_state() || restoration_pending()) return false;
        return !call_ || call_->release_after_quiescence();
    }
};
}
