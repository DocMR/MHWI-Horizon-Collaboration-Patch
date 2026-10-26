#pragma once
#include "stormbolt_owned_call_site.h"

namespace stormslinger {
// PS49A13EB/140E selects code15 for ammo40 before the spread table lookup.
// Only the corresponding PC firing CALL is replaced; other callers of the
// original recoil helper and all ordinary ammo retain native behavior.
class StormboltRecoilAmmo40Patch {
    using Query=std::uint32_t(*)(void*,std::uint32_t);
    StormboltOwnedCallSite site_;
    Query original_{};
    inline static StormboltRecoilAmmo40Patch* owner_{};
    static std::uint32_t query(void* weapon,std::uint32_t ammo) {
        if(ammo==40) return 15;
        return owner_->original_(weapon,ammo);
    }
public:
    explicit StormboltRecoilAmmo40Patch(HookTransaction* transaction):site_(transaction) {}
    bool prepare(void* site) {
        if(owner_ || !site) return false;
        // Original214D03D CALL214DE10: target = return +0xDCE.
        original_=reinterpret_cast<Query>(reinterpret_cast<std::uintptr_t>(site)+5+0xDCE);
        owner_=this;
        return site_.prepare(site,reinterpret_cast<std::uintptr_t>(&query),
            std::array<unsigned char,5>{0xE8,0xCE,0x0D,0x00,0x00});
    }
    bool activate() {return site_.activate();}
    bool restore() {return site_.restore();}
    bool active() const noexcept {return site_.active();}
    bool release_after_quiescence() {
        if(!site_.release_after_quiescence()) return false;
        if(owner_==this) owner_=nullptr;
        return true;
    }
};
}
