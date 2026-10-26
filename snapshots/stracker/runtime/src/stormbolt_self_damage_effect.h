#pragma once
#include <cstdint>

namespace stormslinger {
// PC CCC620: component, equipment slot, group, record, post-enable.
using StormboltEquipmentEffectRequest = void* (*)(void*,std::uint32_t,
    std::uint32_t,std::uint32_t,bool);
struct StormboltSelfDamageEffectResult {
    bool invoked;
    void* native_result;
};
// Only the final primary-effect request in PS4998CB5..4998CCC.
// Caller has completed preceding state/message/sound operations and validated
// the borrowed primary component, owner and body resource readiness/lifetimes.
// Native owns selector construction, resource lookup and conditional post work.
inline StormboltSelfDamageEffectResult stormbolt_self_damage_effect(
    void* primary,StormboltEquipmentEffectRequest request) {
    if (!primary || !request) return {false,nullptr};
    return {true,request(primary,1,0x7D0,1,true)};
}
} // namespace stormslinger
