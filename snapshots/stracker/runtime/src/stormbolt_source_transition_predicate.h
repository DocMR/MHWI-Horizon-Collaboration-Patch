#pragma once
#include <cstdint>

namespace stormslinger {
// Semantic readers/calls for PS2715660. Bind each against native PC dataflow;
// the direct-input reader covers PS271567B..56BA only (no platform/authority).
// It is a read-only snapshot with no native calls or side effects; the native
// flag08 test precedes the remaining-count MOV in that prefix.
// Caller borrows authenticated transition/weapon lifetimes and serializes access.
struct StormboltSourceTransitionPredicateApi {
    std::int32_t (*remaining)(void*)=nullptr;
    bool (*direct_toggle)(void*)=nullptr;
    bool (*platform_blocked)()=nullptr;
    bool (*authority)()=nullptr;
    std::uint32_t (*source_count)(void*)=nullptr;
    bool (*special_mode)(void*)=nullptr;
    bool (*charge_ratio)(void*,float&)=nullptr; // false if native actor/parameter absent.
    std::int32_t (*command)(void*)=nullptr; // Preserve native side effects, raw result.
    bool (*mine_enabled)(void*)=nullptr;
    // Native PC alternatives carry their singleton context explicitly.
    bool (*native_platform_blocked)()=nullptr;
    void** native_authority_slot=nullptr;
    std::uint8_t (*native_authority_query)(void*,std::uint32_t)=nullptr;
    bool is_platform_blocked() const {
        return platform_blocked ? platform_blocked() : native_platform_blocked();
    }
    bool has_authority() const {
        if(authority) return authority();
        auto* object=*native_authority_slot;
        return object && native_authority_query(object,0)!=0;
    }
    bool complete() const noexcept {
        return remaining && direct_toggle && (platform_blocked || native_platform_blocked) &&
            (authority || (native_authority_slot && native_authority_query)) &&
            source_count && special_mode && charge_ratio && command && mine_enabled;
    }
};

struct StormboltSourceTransitionObservation {
    bool direct=false,command_called=false;
    std::int32_t command_result=-1;
};

// Complete decision body after the native weapon is resolved, not a memory
// binding or installed condition hook. Native calls stay outside sidecar locks.
inline std::int32_t stormbolt_evaluate_source_transition(void* transition,void* weapon,
    const StormboltSourceTransitionPredicateApi& api,
    StormboltSourceTransitionObservation* observation=nullptr) {
    if(observation) *observation={};
    if(!transition || !weapon || !api.complete()) return -1;
    const bool direct=api.direct_toggle(transition);
    if(observation) observation->direct=direct;
    const auto remaining=api.remaining(weapon); // Captured before native calls.
    if(direct && !api.is_platform_blocked() && api.has_authority() &&
        api.source_count(weapon) && api.special_mode(weapon)) {
        float ratio{};
        // PS UCOMISS zero,ratio: positive OR unordered takes the general path.
        // An absent actor/parameter instead returns3 immediately.
        if(!api.charge_ratio(weapon,ratio) || ratio<=0.0f) return 3;
    }
    if(!remaining && (api.is_platform_blocked() || !api.has_authority() || !api.source_count(weapon)))
        return -1;
    const auto command_result=api.command(transition);
    if(observation) {observation->command_called=true;observation->command_result=command_result;}
    if(command_result==-1) return -1;
    // Re-read every gate after the command callback, just as the original does.
    if(!api.is_platform_blocked() && api.has_authority() && api.source_count(weapon))
        return api.special_mode(weapon) ? 3 : 2;
    return api.mine_enabled(weapon) ? 1 : 0;
}
} // namespace stormslinger
