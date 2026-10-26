#pragma once
#include "stormbolt_power_cell_store.h"

namespace stormslinger {
// PS2C53024..2C530F3: producer of status+A14. Native uses count!=threshold,
// including above-threshold states, rather than the event's count<threshold.
// Actor flag outputs in the producer are a separate integration requirement.
inline void update_stormbolt_power_cell_event_eligibility(StormboltPowerCellState& state,
    bool ownerFlag,bool indexPresent,std::int16_t resourceCount,
    std::uint32_t threshold) noexcept {
    state.event_eligible=ownerFlag && indexPresent && resourceCount>0 && state.count!=threshold;
}
} // namespace stormslinger
