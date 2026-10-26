#pragma once
#include "stormbolt_charge_hud.h"
#include "stormbolt_shell_hook.h"
#include "stormbolt_update_gate.h"

namespace stormslinger {
struct StormboltChargeHudIdentity {
    std::uintptr_t controller{}, actor{}, weapon{}, parameter{};
    StormboltUpdateGateApi authority{};
};
// Called only inside the live native controller refresh. Checks identity and
// applicability, not native lifetime. The host must establish synchronous borrow.
inline bool stormbolt_charge_hud_actor_input(std::uintptr_t actor,
    const StormboltChargeHudIdentity& expected,StormboltChargeHudInput& output) {
    std::uintptr_t vt=0,weapon=0,owner=0,parameter=0;
    std::uint8_t alive=0,enabled=0,drawn=0;
    std::uint32_t category=0,id=0,part=0,ammo=0;
    if(!expected.actor || !expected.weapon || !expected.parameter ||
        !read_stormbolt_field(actor,0,vt) || vt!=expected.actor ||
        !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
        !read_stormbolt_field(actor,0x76B0,weapon) ||
        !read_stormbolt_field(weapon,0,vt) || vt!=expected.weapon ||
        !read_stormbolt_field(weapon,0xC,alive) || !(alive&0xE) ||
        !read_stormbolt_field(weapon,0x9F8,category) || category!=13 ||
        !read_stormbolt_field(weapon,0x9FC,id) || (id!=238 && id!=239) ||
        !read_stormbolt_field(weapon,0x9B0,owner) || owner!=actor ||
        !read_stormbolt_field(weapon,0x20E0,enabled) || !enabled ||
        !read_stormbolt_field(weapon,0x2388,part) || !part ||
        !expected.authority.authority_slot || !expected.authority.query) return false;
    void* authority=nullptr;
    // PS19DCBFB/CCD8 and43A3E9C/3EAD use sPlatformCtrl+5C and
    // sAuthority+11. The established PC equivalent is query5DC320(index0):
    // its platform check240720 is the native false stub, then it reads+38.
    // Reuse that native query; PS singleton offsets are not a PC layout.
    if(!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(expected.authority.authority_slot),0,authority) ||
        !authority || !expected.authority.query(authority,0)) return false;
    if(!read_stormbolt_field(weapon,0x27C8,ammo) ||
        !read_stormbolt_field(actor,0x76A8,drawn)) return false;
    StormboltChargeHudInput result{};
    result.special_ammo_selected=ammo==40;
    result.weapon_drawn=drawn!=0;
    // Match the original's absent-owner-config path: a visible empty bar.
    if(read_stormbolt_field(actor,0x7810,parameter) && parameter &&
        read_stormbolt_field(parameter,0,vt) && vt==expected.parameter &&
        read_stormbolt_field(weapon,0x2358,result.charge) &&
        read_stormbolt_field(parameter,0x9C0,result.maximum_charge))
        result.configuration_available=true;
    output=result;
    return true;
}
inline bool stormbolt_charge_hud_native_input(void* controller,
    const StormboltChargeHudIdentity& expected,StormboltChargeHudInput& output) {
    const auto self=reinterpret_cast<std::uintptr_t>(controller);
    std::uintptr_t vt=0,actor=0;
    return expected.controller && read_stormbolt_field(self,0,vt) && vt==expected.controller &&
        read_stormbolt_field(self,0x58,actor) &&
        stormbolt_charge_hud_actor_input(actor,expected,output);
}
} // namespace stormslinger
