#include "stormbolt_instance_store.h"
#include "stormbolt_coefficient_adapter.h"
#include "generated_stormbolt_rows.h"
#include "stormbolt_final_action_core.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <limits>

int main() {
    using namespace stormslinger;
    {
        for (unsigned mask = 0; mask < 16; ++mask) {
            std::array<bool, 4> active{};
            for (unsigned i = 0; i < 4; ++i) active[i] = (mask & (1u << i)) != 0;
            StormboltFinalActionState action{};
            StormboltFinalActorFlags actor_flags{0x100u, 1};
            std::uint32_t events = 0xC0000001;
            assert(prepare_stormbolt_final_post_step(action, events, actor_flags, active)
                == (mask == 0));
            assert(events == 0x80000001 && actor_flags.ps8714 == 0x2100u
                && actor_flags.ps8745 == 0x21);
            mark_stormbolt_final_firing_returned(action);
            actor_flags = {0x100u, 1}; events = 0x40000000;
            assert(prepare_stormbolt_final_post_step(action, events, actor_flags, active)
                == (mask == 0));
            assert(events == 0 && actor_flags.ps8714 == 0x100u && actor_flags.ps8745 == 1);
            events = 1;
            assert(!prepare_stormbolt_final_post_step(action, events, actor_flags, active)
                && events == 1);
        }
    }
    {
        StormboltFinalActionState action{};
        StormboltStageState weapon{};
        StormboltStageTiming action_timing{};
        action_timing.action_row[1] = 0.6f;
        action_timing.post_shot_decay = 0.5f;
        std::uint32_t flags = 0x40000001;
        assert(prepare_stormbolt_final_step(action, weapon, flags, 31, 0, action_timing)
            == StormboltFinalStep::waiting);
        assert(action.phase == 0 && weapon.counter == 0 && flags == 0x40000001);
        assert(prepare_stormbolt_final_step(action, weapon, flags, -30, 0, action_timing)
            == StormboltFinalStep::armed);
        assert(action.phase == 1 && weapon.counter == 0 && flags == 0x40000001);
        assert(prepare_stormbolt_final_step(action, weapon, flags, 999, 0, action_timing)
            == StormboltFinalStep::firing_requested);
        assert(weapon.counter == 1 && weapon.shot_snapshot == 0 && flags == 0x40000000
            && weapon.action_timer == 0.6f && !action.firing_call_returned);
        mark_stormbolt_final_firing_returned(action);
        assert(action.firing_call_returned);
        assert(prepare_stormbolt_final_step(action, weapon, flags, 0, 0, action_timing)
            == StormboltFinalStep::waiting);
        flags |= 1;
        // PS does not gate repeated event consumption on the firing-returned flag.
        assert(prepare_stormbolt_final_step(action, weapon, flags, 0, 0, action_timing)
            == StormboltFinalStep::firing_requested);
        assert(weapon.counter == 2 && weapon.shot_snapshot == 1);
        action.phase = 2; flags |= 1;
        assert(prepare_stormbolt_final_step(action, weapon, flags, 0, 0, action_timing)
            == StormboltFinalStep::waiting && (flags & 1));
        action.phase = 1;
        action_timing.post_shot_decay = (std::numeric_limits<float>::quiet_NaN)();
        assert(prepare_stormbolt_final_step(action, weapon, flags, 0, 0, action_timing)
            == StormboltFinalStep::invalid && (flags & 1) && weapon.counter == 2);
    }
    StormboltInstanceStore<2> store;
    assert(!store.initialize(0));
    auto first = store.initialize(0x1000), second = store.initialize(0x2000);
    assert(first && second);
    assert(!store.initialize(0x3000)); // Capacity never evicts active state.
    assert(store.transact(first, [](auto& state) noexcept { state.counter = 7; return true; }));
    StormboltStageState state{};
    assert(store.snapshot(second, state) && state.counter == 0);
    StormboltStageTiming timing{};
    timing.action_row[0] = 0.5f;
    timing.post_shot_decay = 0.5f;
    assert(store.transact(first, [](auto& value) noexcept {
        value.shot_snapshot = 8; value.action_timer = 2;
        value.decay_timer = 3; return true;
    }));
    assert(store.transact(first, [&](auto& value) noexcept {
        return reset_stormbolt_final_action(value, timing, false);
    }));
    assert(store.snapshot(first, state) && state.counter == 7 && state.action_timer == 2);
    auto invalid_timing = timing;
    invalid_timing.post_shot_decay = (std::numeric_limits<float>::quiet_NaN)();
    assert(!store.transact(first, [&](auto& value) noexcept {
        return reset_stormbolt_final_action(value, invalid_timing, true);
    }));
    assert(store.snapshot(first, state) && state.counter == 7 && state.decay_timer == 3);
    assert(store.transact(first, [&](auto& value) noexcept {
        return reset_stormbolt_final_action(value, timing, true);
    }));
    assert(store.snapshot(first, state) && state.counter == 0 && state.shot_snapshot == 8
        && state.action_timer == 0.5f && state.decay_timer == 0.5f);
    assert(!store.transact(first, [](auto& value) noexcept { value.counter = 99; return false; }));
    assert(store.snapshot(first, state) && state.counter == 0 && state.shot_snapshot == 8);
    auto replacement = store.initialize(first.address);
    assert(replacement && replacement.generation != first.generation);
    bool invoked = false;
    assert(!store.transact(first, [&](auto&) noexcept { invoked = true; return true; }));
    assert(!invoked && !store.retire(first));
    assert(store.snapshot(replacement, state) && state.counter == 0);
    assert(store.retire(replacement));
    auto reused = store.initialize(first.address);
    assert(reused.generation != replacement.generation);
    assert(!store.retire(replacement));
    assert(store.snapshot(reused, state) && state.counter == 0);
    auto increment = [&] {
        for (unsigned i = 0; i < 5000; ++i)
            assert(store.transact(reused, [](auto& value) noexcept { ++value.counter; return true; }));
    };
    std::thread a(increment), b(increment);
    a.join(); b.join();
    assert(store.snapshot(reused, state) && state.counter == 10000);
    assert(store.snapshot(second, state) && state.counter == 0);
    assert(store.transact(reused, [](auto& value) noexcept {
        value.shot_snapshot = 6; return true;
    }));
    std::array<unsigned char, 0x6B0> buffer{};
    assert(store.transact(reused, [&](auto& value) noexcept {
        return apply_stormbolt_coefficients(13, 239, 40, true, buffer.data(),
            buffer.size(), value, ps_stormbolt_rows) == CoefficientApplyResult::applied;
    }));
    float coefficient = 0;
    std::memcpy(&coefficient, buffer.data() + 0x6C, sizeof(coefficient));
    assert(coefficient == ps_stormbolt_rows[6].fields[6]);
    const auto preserved_buffer = buffer;
    assert(store.retire(reused) && store.retire(second));
    assert(!store.snapshot(reused, state));
    assert(!store.transact(reused, [&](auto& value) noexcept {
        return apply_stormbolt_coefficients(13, 239, 40, true, buffer.data(),
            buffer.size(), value, ps_stormbolt_rows) == CoefficientApplyResult::applied;
    }));
    assert(buffer == preserved_buffer);
    std::cout << "PASS: independent state, capacity, rollback, generations, stale retirement, concurrent updates and coefficient integration\n";
}
