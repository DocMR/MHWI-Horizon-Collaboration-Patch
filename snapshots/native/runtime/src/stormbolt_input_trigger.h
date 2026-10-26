#pragma once
#include "stormbolt_stage_core.h"
#include <cstring>

namespace stormslinger {
struct StormboltInputTriggerApi {
    bool (*primary)(void*,std::uint32_t)=nullptr; // PC1230070.
    bool (*alternate_gate)(void*,std::uint32_t)=nullptr; // PC1230000 includes E44C mode.
};
// Both native predicates take the input subobject (actor12608 +10), index27.
// The second predicate is conditional on primary, matching PS7F9B..7FC7.
inline bool stormbolt_read_input_trigger(void* actor,const StormboltInputTriggerApi& api,
    bool& permitted) {
    if(!actor || !api.primary || !api.alternate_gate) return false;
    void* input{};
    std::memcpy(&input,static_cast<const unsigned char*>(actor)+0x12608,sizeof(input));
    if(!input) return false;
    auto* subobject=static_cast<unsigned char*>(input)+0x10;
    const bool result=api.primary(subobject,0x27) && api.alternate_gate(subobject,0x27);
    permitted=result;
    return true;
}
// Logical inputs from PS7F90..7FC7, deliberately not PC memory offsets.
// The native reader above combines primary and the conditional alternate gate.
struct StormboltInputTrigger {
    bool primary=false;   // PS input byte0D bit20 OR byte21 bit08.
    bool alternate=false;// PS input byte6D bit20 OR byte81 bit08.
    bool alternate_required=false; // PS actor byteE49C !=0.
    float resource=0;     // PS weapon2394 -> PC2358, read below.
};
// Borrowed authenticated PC weapon object. Preserve raw float representation;
// resource depletion/refill remains the responsibility of native game updates.
inline bool stormbolt_read_input_resource(const void* weapon,float& output) noexcept {
    if(!weapon) return false;
    std::memcpy(&output,static_cast<const unsigned char*>(weapon)+0x2358,sizeof(output));
    return true;
}
// False continues at8161; true continues with effect selection at7FFC.
// A negative or unordered action timer is retained, not a rejection condition.
inline bool stormbolt_advance_input_trigger(StormboltStageState& state,
    const StormboltInputTrigger& input,float step) noexcept {
    if(!input.primary || (input.alternate_required && !input.alternate) ||
       !(input.resource>0.0f)) return false;
    if(state.action_timer>=0.0f) state.action_timer-=step;
    return true;
}
} // namespace stormslinger
