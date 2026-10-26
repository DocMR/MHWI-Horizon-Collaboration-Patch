#pragma once
#include "stormbolt_owned_call_site.h"

extern "C" void stormbolt_elemental_class48_entry();
extern "C" void stormbolt_elemental_ammo_class48_entry();
namespace stormslinger {
// Same shared transaction/quiescence contract as other replay CALL sites.
// Only extends native category membership; damage arithmetic remains native.
class StormboltElementalClass48Patch {
    StormboltOwnedReplayCallSite<8> site_;
    StormboltOwnedReplayCallSite<8> ammo_site_;
    bool ammo_enabled_=false;
public:
    explicit StormboltElementalClass48Patch(HookTransaction* transaction)
        : site_(transaction),ammo_site_(transaction) {}
    bool prepare(void* site,void* ammo_site=nullptr) {
        ammo_enabled_=ammo_site!=nullptr;
        return (!ammo_enabled_ || ammo_site_.prepare(ammo_site,
            reinterpret_cast<std::uintptr_t>(&stormbolt_elemental_ammo_class48_entry),
            std::array<unsigned char,8>{0x83,0xF8,0x1F,0x75,0x76,0x48,0x85,0xF6})) && site_.prepare(site,
            reinterpret_cast<std::uintptr_t>(&stormbolt_elemental_class48_entry),
            std::array<unsigned char,8>{0x83,0xF8,0x05,0x74,0x08,0x83,0xC0,0xE4});
    }
    bool activate() { return site_.activate() && (!ammo_enabled_ || ammo_site_.activate()); }
    bool restore() { return (!ammo_enabled_ || ammo_site_.restore()) && site_.restore(); }
    bool active() const noexcept { return site_.active() || ammo_site_.active(); }
    bool release_after_quiescence() { return (!ammo_enabled_ || ammo_site_.release_after_quiescence()) && site_.release_after_quiescence(); }
};
}
