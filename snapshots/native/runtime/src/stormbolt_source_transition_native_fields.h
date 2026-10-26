#pragma once
#include "stormbolt_source_transition_predicate.h"
#include <cstring>

namespace stormslinger {
// Authenticated field pairs in stormbolt-source-fields-focus-audit.json and
// stormbolt-input-resource-binding.json. Caller borrows native live objects;
// these readers neither establish lifetime nor change any native state.
template<class T> inline T stormbolt_source_field(const void* object,std::size_t offset) noexcept {
    T value{};std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,sizeof(value));return value;
}
inline std::int32_t stormbolt_source_remaining(void* weapon) noexcept {
    return stormbolt_source_field<std::int32_t>(weapon,0x2848);
}
inline std::uint32_t stormbolt_source_count(void* weapon) noexcept {
    return stormbolt_source_field<std::uint32_t>(weapon,0x2388);
}
inline bool stormbolt_source_special_mode(void* weapon) noexcept {
    return stormbolt_source_field<std::uint8_t>(weapon,0x2350)!=0;
}
// PS271567B..56BA paired with native PC1230070/1230000,1F62080/B0.
// Logical command20 maps to physical bit31; CB203 is the native fallback.
// Context/input pointers must be authenticated by the outer readiness check.
inline bool stormbolt_source_direct_toggle(void* transition) noexcept {
    if(!(stormbolt_source_field<std::uint8_t>(transition,8)&1)) return false;
    auto* input=stormbolt_source_field<void*>(transition,0x20);
    auto* actor=stormbolt_source_field<void*>(input,0x30);
    auto* physical=stormbolt_source_field<void*>(actor,0xE480);
    const bool primary=stormbolt_source_field<std::int32_t>(physical,8)<0 ||
        (stormbolt_source_field<std::uint8_t>(physical,0x21)&8)!=0;
    if(!primary) return false;
    if(!stormbolt_source_field<std::uint8_t>(actor,0xE44C)) return true;
    return stormbolt_source_field<std::int32_t>(physical,0x68)<0 ||
        (stormbolt_source_field<std::uint8_t>(physical,0x81)&8)!=0;
}
inline bool stormbolt_source_charge_ratio(void* weapon,float& output) noexcept {
    auto* actor=stormbolt_source_field<void*>(weapon,0x9B0);
    if(!actor || !(stormbolt_source_field<std::uint8_t>(actor,0xC)&0xE)) return false;
    auto* parameter=stormbolt_source_field<void*>(actor,0x7810);
    if(!parameter) return false;
    const auto charge=stormbolt_source_field<float>(weapon,0x2358);
    const auto limit=stormbolt_source_field<float>(parameter,0x9C0);
    output=charge/limit; // Preserve native zero/NaN/infinity, no clamp.
    return true;
}
inline StormboltSourceTransitionPredicateApi stormbolt_source_native_field_api(
    StormboltSourceTransitionPredicateApi api={}) noexcept {
    api.remaining=&stormbolt_source_remaining;
    api.direct_toggle=&stormbolt_source_direct_toggle;
    api.source_count=&stormbolt_source_count;
    api.special_mode=&stormbolt_source_special_mode;
    api.charge_ratio=&stormbolt_source_charge_ratio;
    return api;
}
} // namespace stormslinger
