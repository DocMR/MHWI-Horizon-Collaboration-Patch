#pragma once
#include "stormbolt_effect_record_core.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct Stormbolt2011RecordSelection {
    std::uint32_t ordinary_record = 0;
    bool terminal_requested = false;
    std::uint32_t terminal_record = ps_stormbolt_2011_terminal_record;
};
// Read sidecar state only. Caller must validate native lifetime and branch gates
// before issuing requests, outside the sidecar lock. This does not authorize
// both branches, or bind an unverified global provider/effect identifier.
template<std::size_t Capacity>
inline bool snapshot_stormbolt_2011_records(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token, Stormbolt2011RecordSelection& output) {
    StormboltStageState state{};
    if (!store.snapshot(token, state)) return false;
    output = {select_stormbolt_2011_ordinary_record(state.counter),
              stormbolt_2011_terminal_record_requested(state.shot_snapshot),
              ps_stormbolt_2011_terminal_record};
    return true;
}
} // namespace stormslinger
