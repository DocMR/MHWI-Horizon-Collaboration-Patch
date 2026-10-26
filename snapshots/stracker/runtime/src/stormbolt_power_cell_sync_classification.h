#pragma once
#include "stormbolt_power_cell_remote_decision.h"
#include "stormbolt_power_cell_sync_event.h"

namespace stormslinger {
// PS producer2C34152..3426A. The first two flags represent count bands;
// the receiver restores representative counts0/threshold-1, not exact count.
inline StormboltPowerCellRemoteFlags classify_stormbolt_power_cell_sync(
    bool powered,std::uint32_t count,std::uint32_t threshold,std::uint32_t window,
    const StormboltPowerCellSyncEventState& event) noexcept {
    StormboltPowerCellRemoteFlags flags{};
    flags.effect=snapshot_stormbolt_power_cell_sync_event(event);
    if (!powered) return flags;
    if (count==threshold) flags.reached=true;
    else {
        const std::uint32_t boundary=threshold-window;
        flags.empty=count<boundary;
        flags.below=count>=boundary;
    }
    return flags;
}
} // namespace stormslinger
