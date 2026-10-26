#pragma once
#include "stormbolt_power_cell_loop_state.h"
#include "stormbolt_power_cell_sound.h"
#include "stormbolt_loop_native_context.h"

namespace stormslinger {
// Called only after the highest-stage promotion/prompt and a live component
// lookup. The caller authenticates container, actor and native defaults.
template<std::size_t Capacity>
bool start_stormbolt_power_cell_loop_request(
    StormboltPowerCellLoopStore<Capacity>& store,StormboltPowerCellLoopToken token,
    void* container,void* actor,
    const std::array<std::array<float,3>,3>& defaults,
    StormboltNamedEventRequest request) {
    if (!store.transact(token,[](auto& state) noexcept {
        state.timer=5.0f;
        return true;
    })) return false;
    if (!actor || !container || !request) return false;
    // Request result does not control the native active latch. Never execute
    // game code under the store mutex, or recreate a retired generation.
    (void)request_stormbolt_power_cell_loop_play(container,actor,defaults,request);
    return store.transact(token,[](auto& state) noexcept {
        state.active=true;
        return true;
    });
}
// update is a pure, noexcept state operation selecting an authenticated native
// branch. It receives actor availability and a STOP marker, never a game API.
template<std::size_t Capacity,class Update>
bool update_stormbolt_power_cell_loop_request(
    StormboltPowerCellLoopStore<Capacity>& store,StormboltPowerCellLoopToken token,
    Update&& update,void* container,void* actor,
    const std::array<std::array<float,3>,3>& defaults,
    const std::array<float,3>& position,StormboltNamedEventRequest request) {
    const bool available=container && actor && request;
    bool stopping=false;
    if (!store.transact(token,[&](auto& state) noexcept {
        const bool active=state.active;
        const auto mark=[&]() noexcept { stopping=true;return false; };
        static_assert(noexcept(update(state,available,mark)),
                      "Loop state decision must not throw or invoke native code");
        update(state,available,mark);
        if (stopping) state.active=active; // Native clears only after STOP.
        return true;
    })) return false;
    if (!stopping) return true;
    (void)request_stormbolt_power_cell_loop_stop(container,actor,defaults,position,request);
    return store.transact(token,[](auto& state) noexcept {
        state.active=false;
        return true;
    });
}
template<std::size_t Capacity>
bool process_stormbolt_power_cell_loop_audio(
    StormboltPowerCellLoopStore<Capacity>& store,StormboltPowerCellLoopToken token,
    const StormboltLoopNativeContext& context,
    const StormboltPowerCellLoopUpdateInputs& input,StormboltNamedEventRequest request) {
    const auto decision=[&](auto& state,bool available,auto& stop) noexcept {
        update_stormbolt_power_cell_loop_audio(state,input,context.step,available,stop);
    };
    return update_stormbolt_power_cell_loop_request(store,token,decision,context.container,
        context.actor_available ? context.actor : nullptr,context.defaults,context.position,request);
}
} // namespace stormslinger
