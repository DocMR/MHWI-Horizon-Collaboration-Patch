#pragma once
#include <cstdint>
#include <cstring>
#include "stormbolt_stage_core.h"

namespace stormslinger {
struct StormboltUpdateGateApi {
    void** authority_slot=nullptr; // PC4CD2AC0, authenticated borrowed singleton.
    std::uint8_t (*query)(void*,std::uint32_t)=nullptr; // PC5DC320, index zero.
};
// PS4996FBA..FD3. These categories select the cleanup branch, not a generic
// denial of all weapon updates. Missing action returns descriptor zero natively.
inline bool stormbolt_action_selects_cleanup(std::uint64_t descriptor) noexcept {
    switch (descriptor & 0x3F) {
    case 5: case 12: case 15: case 28: case 36: return true;
    default: return false;
    }
}
// Pure post-authority decision, PS4996F5A..6FE9 and 4997494..74A7.
// Caller supplies a descriptor only after mode/actor checks permit its lookup;
// this function performs no native calls and does not replace their ordering.
enum class StormboltStageRoute { cleanup, high_stage, local_input, remote_effect, remote_decay };
inline StormboltStageRoute stormbolt_select_stage_route(StormboltStageState& state,
    bool local_owner,bool special_mode,bool actor_enabled,std::uint64_t descriptor) noexcept {
    if (local_owner) state.input_effect_active=0;
    if (!special_mode || !actor_enabled || stormbolt_action_selects_cleanup(descriptor))
        return StormboltStageRoute::cleanup;
    if (state.counter>=9) return StormboltStageRoute::high_stage;
    if (local_owner) return StormboltStageRoute::local_input;
    return state.input_effect_active ? StormboltStageRoute::remote_effect : StormboltStageRoute::remote_decay;
}
using StormboltActionDescriptor = std::uint64_t (*)(void*,std::uint32_t); // PC1BFF6F0, channel0.
enum class StormboltLocalInputPath { decay_branch, additional_input_checks };
struct StormboltLocalInputApi {
    bool (*query)(void*,std::uint32_t)=nullptr; // PC122FD90, input subobject+10.
};
// Read the single input bit required by the local split. This is not a copy of
// the entire PS flag word. Caller borrows actor/input lifetimes and verifies ABI.
inline bool stormbolt_read_local_input_flags(void* actor,
    const StormboltLocalInputApi& api,std::uint32_t& output) {
    if (!actor || !api.query) return false;
    void* input{};
    std::memcpy(&input,static_cast<const unsigned char*>(actor)+0x12608,sizeof(input));
    if (!input) return false;
    const bool present=api.query(static_cast<unsigned char*>(input)+0x10,0x78);
    output=present ? 0x01000000u : 0u;
    return true;
}
// PS4997828..785D only, after local-owner/action/actor gates. The caller must
// supply authenticated input flags; the PS input-object offset is NOT a PC ABI.
// additional_input_checks leads to 4997F90, not directly to firing or charging.
inline StormboltLocalInputPath stormbolt_local_input_path(std::uint32_t flags,
    std::uint32_t group,std::uint32_t index) noexcept {
    if (!(flags&0x01000000u) || group!=3)
        return StormboltLocalInputPath::decay_branch;
    if (index>63) return StormboltLocalInputPath::additional_input_checks;
    return (0xFF8FF801C0000000ull>>index)&1
        ? StormboltLocalInputPath::decay_branch
        : StormboltLocalInputPath::additional_input_checks;
}
enum class StormboltUpdateGateResult { unavailable, inactive, continue_checks };
// Only the common authority/count prefix. Passing this is NOT permission to
// decay: action, input, effect cleanup and sidecar-state branches remain needed.
// Run after the original weapon update refreshes its equipment count array.
inline StormboltUpdateGateResult stormbolt_update_common_gate(void* weapon,
    const StormboltUpdateGateApi& api) {
    if (!weapon || !api.authority_slot || !api.query || !*api.authority_slot)
        return StormboltUpdateGateResult::unavailable;
    if (!api.query(*api.authority_slot,0)) return StormboltUpdateGateResult::inactive;
    std::uint32_t count{};
    std::memcpy(&count,static_cast<const unsigned char*>(weapon)+0x2388,sizeof(count));
    return count ? StormboltUpdateGateResult::continue_checks : StormboltUpdateGateResult::inactive;
}
} // namespace stormslinger
