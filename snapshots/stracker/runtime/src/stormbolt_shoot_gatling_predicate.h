#pragma once
#include <cstdint>

namespace stormslinger {
// PS2715810: readonly condition for the separately registered ShootGatling type.
// Native PC readers and sidecar lifetime validation are supplied by its adapter.
struct StormboltShootGatlingInput {
    std::uint32_t ammo=0,stage=0;
    bool held=false;
    float resource=0;
    bool owner_blocked=false;
    float action_timer=0;
    bool owner_live=false,primary_fire=false,fallback_fire=false;
    std::uint8_t stage_mode=0;
};
inline std::int32_t stormbolt_evaluate_shoot_gatling(const StormboltShootGatlingInput& input) noexcept {
    if(input.ammo!=40 || input.stage>8 || !input.held) return -1;
    // Native UCOMISS zero,resource; JAE rejects <=0, but permits unordered.
    if(input.resource<=0.0f || input.owner_blocked || input.action_timer>0.0f) return -1;
    if(input.owner_live && !input.primary_fire && !input.fallback_fire) return -1;
    if(input.stage==8) return 3;
    // CMP byte,1 / SBB eax,0: only zero selects1; every nonzero byte selects2.
    return input.stage_mode==0 ? 1 : 2;
}
} // namespace stormslinger
