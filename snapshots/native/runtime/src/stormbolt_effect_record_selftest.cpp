#include "stormbolt_effect_record_adapter.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace stormslinger;
int main() {
    const std::uint32_t expected[] = {0,0,0,1,1,1,2,2,2,0};
    for (std::uint32_t counter = 0; counter < 10; ++counter)
        assert(select_stormbolt_2011_ordinary_record(counter) == expected[counter]);
    assert(select_stormbolt_2011_ordinary_record(UINT32_MAX) == 0);
    assert(stormbolt_2011_terminal_record_requested(8));
    assert(!stormbolt_2011_terminal_record_requested(9));
    assert(!stormbolt_2011_terminal_record_requested(UINT32_MAX));
    // Divergent fields are intentional: snapshot8 selects terminal record3,
    // current counter9 selects ordinary default0, never a shared clamp-to8.
    assert(select_stormbolt_2011_ordinary_record(9) == 0);
    assert(ps_stormbolt_2011_terminal_record == 3);
    StormboltInstanceStore<2> store;
    auto first = store.initialize(0x1000), second = store.initialize(0x2000);
    assert(store.transact(first, [](StormboltStageState& state) noexcept {
        state.counter = 9; state.shot_snapshot = 8; return true;
    }));
    Stormbolt2011RecordSelection one{}, two{};
    assert(snapshot_stormbolt_2011_records(store, first, one));
    assert(one.ordinary_record == 0 && one.terminal_requested);
    assert(snapshot_stormbolt_2011_records(store, second, two));
    assert(two.ordinary_record == 0 && !two.terminal_requested);
    auto replacement = store.initialize(0x1000);
    one.ordinary_record = 77;
    assert(!snapshot_stormbolt_2011_records(store, first, one));
    assert(one.ordinary_record == 77); // stale token leaves caller output unchanged
    assert(snapshot_stormbolt_2011_records(store, replacement, one));
    assert(!one.terminal_requested);
    assert(store.retire(second));
    assert(!snapshot_stormbolt_2011_records(store, second, two));
    std::cout << "Effect record boundary and divergent-state checks passed.\n";
}
