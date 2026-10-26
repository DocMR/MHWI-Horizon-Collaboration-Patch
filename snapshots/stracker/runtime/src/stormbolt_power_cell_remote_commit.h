#pragma once
#include "stormbolt_power_cell_remote_decision.h"
#include "stormbolt_power_cell_store.h"

namespace stormslinger {
// Previous effect comes from the authenticated incoming/history pair. It must
// be captured before the native application function copies current to history.
template<std::size_t Capacity>
bool commit_stormbolt_power_cell_remote(
    StormboltPowerCellToken token, StormboltPowerCellStore<Capacity>& store,
    StormboltPowerCellRemoteFlags flags, bool previous_effect,
    bool owner_enabled, std::uint32_t threshold, bool associated_present,
    bool associated_player, bool associated_local,
    StormboltPowerCellRemoteDecision& output) {
    StormboltPowerCellRemoteDecision decision{};
    if (!store.transact(token,[&](auto& state) noexcept {
        decision=decide_stormbolt_power_cell_remote(flags,previous_effect,
            owner_enabled,state.count,threshold,associated_present,
            associated_player,associated_local);
        state.count=decision.count;
        return true;
    })) return false;
    output=decision;
    return true;
}
} // namespace stormslinger
