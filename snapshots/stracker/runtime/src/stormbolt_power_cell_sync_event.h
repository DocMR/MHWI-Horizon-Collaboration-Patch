#pragma once
namespace stormslinger {
// PS network component5942: growth marks it, each completed producer pass
// clears it, including the unchanged/no-send branch2C34828->2C3447C.
// Lifecycle and PC transport-bit ownership are connected separately.
struct StormboltPowerCellSyncEventState {bool pending=false;};
inline void mark_stormbolt_power_cell_sync_event(StormboltPowerCellSyncEventState& state) noexcept {
    state.pending=true;
}
inline bool snapshot_stormbolt_power_cell_sync_event(const StormboltPowerCellSyncEventState& state) noexcept {
    return state.pending;
}
inline void complete_stormbolt_power_cell_sync_producer(StormboltPowerCellSyncEventState& state) noexcept {
    state.pending=false;
}
} // namespace stormslinger
