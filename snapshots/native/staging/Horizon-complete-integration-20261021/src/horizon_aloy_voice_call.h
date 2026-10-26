#pragma once
#include "horizon_owner_verification.h"
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>

namespace horizon_other {
namespace aloy_voice_detail {
#include "generated_aloy_voice_owners.inc"
}
// PS group0 uses the ordinary gendered voice10. PC retains the unavailable
// special-bank11 path. Correct only that original formatting CALL; preserve
// character IDs, ordinary hunter voices, other full sets and native loading.
class AloyVoiceCall {
    const std::uint8_t* image_=nullptr;
    std::optional<stormslinger::StormboltOwnedCallSite> call_;
    inline static AloyVoiceCall* active_=nullptr;
    static char* format(char* output,const char* pattern,int voice) {
        const auto* image=active_->image_;
        using Native=char*(*)(char*,const char*,...);
        const auto native=reinterpret_cast<Native>(const_cast<std::uint8_t*>(image)+0x24c380);
        if(voice==11 && pattern==reinterpret_cast<const char*>(image+0x33feb28))
            return native(output,reinterpret_cast<const char*>(image+0x33feb08),'f',10);
        return native(output,pattern,voice);
    }
public:
    explicit AloyVoiceCall(const std::uint8_t* image):image_(image) {}
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if(!image_ || active_ || call_ || !transaction.can_add_hooks())return false;
        for(const auto& owner:aloy_voice_detail::kAloyVoiceOwners)
            if(!verify_native_owner(image_,owner))return false;
        active_=this;call_.emplace(&transaction);
        return call_->prepare(const_cast<std::uint8_t*>(image_)+0x1b47ff8,
            reinterpret_cast<std::uintptr_t>(&format),
            std::array<unsigned char,5>{0xe8,0x83,0x43,0x70,0xfe});
    }
    bool activate_after_shared_commit_while_quiescent() {
        return active_==this && call_ && call_->activate();
    }
    bool restore_while_quiescent() {
        if(call_ && !call_->restore())return false;
        if(active_==this)active_=nullptr;
        return true;
    }
    bool release_after_shared_stop() {
        return (!call_ || call_->release_after_quiescence()) && active_!=this;
    }
};
}
