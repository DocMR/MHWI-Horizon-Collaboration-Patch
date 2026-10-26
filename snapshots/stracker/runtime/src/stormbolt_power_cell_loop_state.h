#pragma once
#include <cstdint>
#include "stormbolt_instance_store.h"

namespace stormslinger {
// PS component tail 9D4/9E0 has no allocated PC counterpart. Keep this state
// outside the native component, with the component's authenticated lifetime.
// This is separate from the status-manager skill-duration timer.
struct StormboltPowerCellLoopState {
    float timer=0.0f; // PS constructor 9D4, not visual timer 9D0.
    bool active=false;
    float visual_timer=-1.0f;
    bool visual_active=false; // PS9D8.
    bool growth=false; // PS9DA, cleared after the update.
    std::uint32_t visual_phase=0; // PS9DC.
};
struct StormboltPowerCellLoopToken {
    std::uintptr_t address=0;
    std::uint64_t generation=0;
    explicit operator bool() const noexcept { return address && generation; }
};
// Keyed by cpPlCombineArmorCtrl, not the weapon or status manager.
template<std::size_t Capacity>
using StormboltPowerCellLoopStore=StormboltInstanceStore<Capacity,
    StormboltPowerCellLoopState,StormboltPowerCellLoopToken>;
// PS4998BC4..4998CAE, entered after highest-stage promotion/prompt.
// The native component has already been resolved by the caller.
template<class Play>
bool start_stormbolt_power_cell_loop(
    StormboltPowerCellLoopState& state,bool actorAvailable,Play&& play) {
    state.timer=5.0f;
    if (!actorAvailable) return false;
    (void)play();
    state.active=true;
    return true;
}
// PS2E33ACC/AE3/B00: any of these resource gates failing selects STOP.
// The caller supplies the live resource lookup result; null-owner bypass is
// a different native branch and must not be converted into this STOP path.
template<class Stop>
bool stop_stormbolt_power_cell_loop_for_resource_gate(
    StormboltPowerCellLoopState& state,bool ownerFlag,
    bool resourceIndexPresent,std::int16_t resourceCount,
    bool actorAvailable,Stop&& stop) {
    if (ownerFlag && resourceIndexPresent && resourceCount>0) return false;
    if (!state.active || !actorAvailable) return false;
    (void)stop();
    state.active=false;
    return true;
}
// PS2E33EA9..2E33FC5 and 2E34085..2E34188: both non-equal
// visual bands stop the loop. The band split does not alter audio behavior.
template<class Stop>
bool update_stormbolt_power_cell_loop_non_equal_count(
    StormboltPowerCellLoopState& state,std::uint32_t count,
    std::uint32_t threshold,
    bool actorAvailable,Stop&& stop) {
    if (count==threshold) return false;
    state.timer=-1.0f;
    if (!state.active || !actorAvailable) return false;
    (void)stop();
    state.active=false;
    return true;
}
// PS2E34F90..2E350BE. stop must perform the native STOP request synchronously.
// The caller resolves current actor availability and the native owner+68 step.
template<class Stop>
bool update_stormbolt_power_cell_loop_timer(
    StormboltPowerCellLoopState& state,float nativeStep,
    bool actorAvailable,Stop&& stop) {
    if (!(state.timer>0.0f)) return false;
    state.timer=state.timer-nativeStep;
    if (!(state.timer<0.0f) || !state.active || !actorAvailable) return false;
    (void)stop();
    state.active=false;
    return true;
}
struct StormboltPowerCellLoopUpdateInputs {
    bool owner_flag=false,resource_index_present=false;
    std::int16_t resource_count=0;
    std::uint32_t count=0,threshold=0,related_count=0;
};
// Mirrors the mutually exclusive native audio branches. Visual work remains
// between the count branch and timer branch in the full native implementation.
template<class Stop>
bool update_stormbolt_power_cell_loop_audio(
    StormboltPowerCellLoopState& state,const StormboltPowerCellLoopUpdateInputs& input,
    float step,bool actorAvailable,Stop&& stop) {
    if (!input.owner_flag || !input.resource_index_present || input.resource_count<=0)
        return stop_stormbolt_power_cell_loop_for_resource_gate(state,input.owner_flag,
            input.resource_index_present,input.resource_count,actorAvailable,stop);
    if (input.count!=input.threshold)
        return update_stormbolt_power_cell_loop_non_equal_count(state,input.count,
            input.threshold,actorAvailable,stop);
    return update_stormbolt_power_cell_loop_timer(state,step,actorAvailable,stop);
}
} // namespace stormslinger
