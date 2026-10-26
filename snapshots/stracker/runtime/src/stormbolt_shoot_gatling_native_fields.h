#pragma once
#include "stormbolt_shoot_gatling_predicate.h"
#include "stormbolt_source_transition_native_fields.h"
#include "stormbolt_stage_core.h"

namespace stormslinger {
struct StormboltShootGatlingNativeApi {
    bool (*held_query)(void*,std::uint32_t)=nullptr; // PC122FD90, readonly.
    bool (*physical_primary)(void*,std::uint32_t)=nullptr; // PC1230070, no alternate gate.
    std::uintptr_t parameter_vtable=0;
};
// Caller authenticates borrowed transition/input/actor/weapon/parameter lifetime
// and sidecar generation. Read in original branch order; stage8 never needs rows.
// No native object is enlarged; stage/timer are the existing weapon sidecar.
inline bool stormbolt_read_shoot_gatling_condition(void* transition,void* weapon,
    const StormboltStageState& state,const StormboltShootGatlingNativeApi& api,
    std::int32_t& output) {
    if(!transition || !weapon || !api.held_query || !api.physical_primary) return false;
    auto finish=[&](int value) {output=value;return true;};
    if(stormbolt_source_field<std::uint32_t>(weapon,0x27C8)!=40 || state.counter>8)
        return finish(-1);
    auto* input=stormbolt_source_field<void*>(transition,0x20);
    if(!input) return false;
    if(!api.held_query(input,0x78)) return finish(-1);
    const auto resource=stormbolt_source_field<float>(weapon,0x2358);
    if(resource<=0.0f) return finish(-1);
    auto* owner=stormbolt_source_field<void*>(weapon,0x9B0);
    if(!owner) return false;
    // Native actor flag16D: PC setter1F604B0 uses base8938; PS base86E8.
    if(stormbolt_source_field<std::uint8_t>(owner,0x8965)&0x20) return finish(-1);
    if(state.action_timer>0.0f) return finish(-1);
    if(stormbolt_source_field<std::uint8_t>(owner,0xC)&0xE) {
        auto* controller=stormbolt_source_field<void*>(owner,0x12608);
        if(!controller) return false;
        if(!api.physical_primary(static_cast<unsigned char*>(controller)+0x10,0x27))
            return finish(-1);
    }
    if(state.counter==8) return finish(3);
    auto* parameter=stormbolt_source_field<void*>(owner,0x7810);
    if(!parameter || !api.parameter_vtable ||
        stormbolt_source_field<std::uintptr_t>(parameter,0)!=api.parameter_vtable) return false;
    const auto mode=stormbolt_source_field<std::uint8_t>(parameter,0x1624+40*state.counter);
    return finish(mode==0?1:2);
}
} // namespace stormslinger
