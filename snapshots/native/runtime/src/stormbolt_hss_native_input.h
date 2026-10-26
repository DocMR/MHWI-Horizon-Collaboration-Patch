#pragma once
#include "stormbolt_native_memory.h"

namespace stormslinger {
struct StormboltHssIdentity {std::uintptr_t actor{},weapon{};};
struct StormboltHssNativeInput {
    std::uintptr_t weapon{};
    bool visible{};
    std::uint8_t reload_context{};
};
struct StormboltHssInputBindings {
    bool (*action_filter)(void*,std::uint32_t){}; // PC1F5AE10
};
// PSBDC8A0 samples only the actor's range byte. Admission/visibility belongs
// to BDC970/BDD460 and must not initiate another GUI hide/show here.
inline bool stormbolt_hss_warning_context(std::uintptr_t actor,
    const StormboltHssIdentity& expected,std::uint8_t& context) {
    std::uintptr_t table{};std::uint8_t alive{};
    return expected.actor && read_stormbolt_field(actor,0,table) &&
        table==expected.actor && read_stormbolt_field(actor,0xC,alive) &&
        (alive&0xE) && read_stormbolt_field(actor,0x1361C,context);
}
// PS BDD460/BDC970 and BDC8A0. Actor is synchronously borrowed by the
// native GUI caller; identity checks do not establish object lifetime.
inline bool stormbolt_hss_native_input(std::uintptr_t actor,
    const StormboltHssIdentity& expected,const StormboltHssInputBindings& api,
    StormboltHssNativeInput& output) {
    if(!api.action_filter || !expected.actor || !expected.weapon) return false;
    std::uintptr_t table{},weapon{},owner{};
    std::uint8_t alive{},enabled{},drawn{},reload{};
    std::uint32_t category{},id{},part{},ammo{},group{},index{},aim{},reject{};
    if(!read_stormbolt_field(actor,0,table) || table!=expected.actor ||
        !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
        !read_stormbolt_field(actor,0x76B0,weapon) ||
        !read_stormbolt_field(weapon,0,table) || table!=expected.weapon ||
        !read_stormbolt_field(weapon,0xC,alive) || !(alive&0xE) ||
        !read_stormbolt_field(weapon,0x9B0,owner) || owner!=actor ||
        !read_stormbolt_field(weapon,0x9F8,category) || category!=13 ||
        !read_stormbolt_field(weapon,0x9FC,id) || (id!=238 && id!=239) ||
        !read_stormbolt_field(weapon,0x2388,part) || !part ||
        !read_stormbolt_field(weapon,0x20E0,enabled) ||
        !read_stormbolt_field(weapon,0x27C8,ammo) ||
        !read_stormbolt_field(actor,0x76A8,drawn) ||
        !read_stormbolt_field(actor,0x6274,group) ||
        !read_stormbolt_field(actor,0x6278,index) ||
        !read_stormbolt_field(actor,0x893C,aim) ||
        !read_stormbolt_field(actor,0x8958,reject) ||
        !read_stormbolt_field(actor,0x1361C,reload)) return false;
    // Both original 120-entry WP13 tables identify key119 as
    // WP_13::SHOOT_GATLING_LAST (PC4FFB6D0 / PS6ED7440).
    const bool visible=enabled && !(group==3 && index==119) && ammo==40 &&
        drawn && (aim&0x68)==8 && !(reject&0x10000000) &&
        !api.action_filter(reinterpret_cast<void*>(actor),0);
    output={weapon,visible,reload};
    return true;
}
} // namespace stormslinger
