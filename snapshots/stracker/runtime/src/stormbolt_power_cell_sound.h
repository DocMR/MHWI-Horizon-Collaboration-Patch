#pragma once
#include "stormbolt_named_event_parameters.h"

namespace stormslinger {
// PC2300260: container, signed category, event name, PC parameters.
using StormboltNamedEventRequest=bool (*)(void*,std::int32_t,const char*,
                                        StormboltNamedEventParameters*);
// The caller supplies live native objects and authenticated default vectors.
// Native code owns its requests; this adapter does not allocate or release them.
inline bool request_stormbolt_power_cell_loop_play(
    void* container, void* actor,
    const std::array<std::array<float,3>,3>& defaults,
    StormboltNamedEventRequest request) {
    if (!container || !request) return false;
    StormboltNamedEventParameters parameters{};
    if (!build_stormbolt_actor_event_parameters(actor,defaults,parameters))
        return false;
    return request(container,-1,"PLAY_PL_BODY_068_DMG_R_LP",&parameters);
}
inline bool request_stormbolt_power_cell_loop_stop(
    void* container, void* actor,
    const std::array<std::array<float,3>,3>& defaults,
    const std::array<float,3>& position,
    StormboltNamedEventRequest request) {
    if (!container || !request) return false;
    StormboltNamedEventParameters parameters{};
    if (!build_stormbolt_stop_event_parameters(actor,defaults,position,parameters))
        return false;
    return request(container,-1,"STOP_PL_BODY_068_DMG_R_LP",&parameters);
}
} // namespace stormslinger
