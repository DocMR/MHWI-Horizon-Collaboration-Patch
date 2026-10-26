#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_transition_ammo_adapter.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes = std::array<std::byte, N>;
Bytes<0x2860> g_weapon{};
Bytes<0x8940> g_actor{}, g_other_actor{};
Bytes<0x1610> g_parameter{}, g_other_parameter{};
Bytes<8> g_manager{}, g_other_manager{}, g_character{};
void* g_manager_slot = nullptr;
void* g_expected_manager = nullptr;
StormboltInstanceStore<2> g_store;
StormboltInstanceToken g_token{};
std::vector<int> g_calls;
bool g_change_owner = false, g_clear_gate = false;
int g_throw_at = 0, g_invalidate_at = 0;

template<class T> void put(void* p, std::size_t off, T value) {
    std::memcpy(static_cast<std::byte*>(p)+off, &value, sizeof(value));
}
template<class T> T get(const void* p, std::size_t off) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(p)+off, sizeof(value));
    return value;
}
void observe(int phase) {
    g_calls.push_back(phase);
    // These callbacks must run before mode/ammo change, and outside the lock.
    assert(get<std::uint8_t>(g_weapon.data(), 0x2350) == 0x6D);
    assert(get<std::uint32_t>(g_weapon.data(), 0x27C8) == 0x6D6D6D6D);
    StormboltStageState state{};
    assert(g_store.snapshot(g_token, state));
    assert(state.counter == 7 && state.shot_snapshot == 5 && state.decay_timer == 3.25f);
    if (g_invalidate_at == phase) {
        assert(g_store.retire(g_token));
        assert(g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data())));
    }
    if (g_throw_at == phase) throw std::runtime_error("native save callback");
}
void* current_character(void* manager) {
    assert(manager == g_expected_manager);
    observe(1);
    return g_character.data();
}
void set_bit(void* character, std::int32_t bit) {
    assert(character == g_character.data() && bit == 11);
    observe(2);
    if (g_change_owner) put(g_weapon.data(), 0x9B0, g_other_actor.data());
    if (g_clear_gate) {
        auto* actor = get<void*>(g_weapon.data(), 0x9B0);
        put(actor, 0x8938, std::uint8_t{0xA4});
    }
}
void prepare(bool gate) {
    g_weapon.fill(std::byte{0x6D});
    g_actor.fill(std::byte{0xA4});
    g_other_actor.fill(std::byte{0xA5});
    g_parameter.fill(std::byte{0x13});
    g_other_parameter.fill(std::byte{0x24});
    put(g_weapon.data(), 0x9B0, g_actor.data());
    put(g_actor.data(), 0x8938, static_cast<std::uint8_t>(gate ? 0xA5 : 0xA4));
    put(g_actor.data(), 0x7810, g_parameter.data());
    put(g_other_actor.data(), 0x7810, g_other_parameter.data());
    put(g_parameter.data(), 0x1608, 1.75f);
    put(g_other_parameter.data(), 0x1608, 2.5f);
    g_token = g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data()));
    assert(g_token);
    assert(g_store.transact(g_token, [](StormboltStageState& s) noexcept {
        s = {7, 5, 8.75f, 3.25f}; return true;
    }));
    g_manager_slot = g_manager.data();
    g_expected_manager = g_manager.data();
    g_calls.clear();
    g_change_owner = g_clear_gate = false;
    g_throw_at = g_invalidate_at = 0;
}
StormboltTransitionSaveApi api() {
    return {&g_manager_slot, current_character, set_bit};
}
}

int main() {
    unsigned combinations = 0;
    for (bool initial : {false, true}) for (bool changed : {false, true})
        for (bool cleared : {false, true}) {
        prepare(initial);
        g_change_owner = changed;
        g_clear_gate = cleared;
        auto mutable_api = api();
        StormboltTransitionAmmoAdapter<2> adapter(g_store, mutable_api);
        mutable_api = {}; // The adapter owns a copy of its function table.
        g_manager_slot = g_other_manager.data();
        g_expected_manager = g_other_manager.data();
        auto expected = g_weapon;
        if (initial && changed) put(expected.data(), 0x9B0, g_other_actor.data());
        put(expected.data(), 0x2350, std::uint8_t{1});
        put(expected.data(), 0x27C8, std::uint32_t{40});
        assert(adapter.apply(g_weapon.data(), g_token) == StormboltTransitionAmmoResult::completed);
        assert(expected == g_weapon); // All unrelated native weapon bytes preserved.
        assert(g_calls == (initial ? std::vector<int>{1, 2} : std::vector<int>{}));
        StormboltStageState state{};
        assert(g_store.snapshot(g_token, state));
        assert(state.counter == (initial && !cleared ? 0u : 7u));
        assert(state.action_timer == (initial && !cleared ? (changed ? 2.5f : 1.75f) : 8.75f));
        assert(state.shot_snapshot == 5 && state.decay_timer == 3.25f);
        ++combinations;
    }
    // Preserve the native DWORD copy, including negative zero/NaN bit patterns.
    for (std::uint32_t bits : {0x80000000u, 0x7FC01234u, 0x3F200000u}) {
        prepare(true);
        put(g_parameter.data(), 0x1608, bits);
        StormboltTransitionAmmoAdapter<2> adapter(g_store, api());
        assert(adapter.apply(g_weapon.data(), g_token) == StormboltTransitionAmmoResult::completed);
        StormboltStageState state{};
        assert(g_store.snapshot(g_token, state));
        assert(get<std::uint32_t>(&state.action_timer, 0) == bits);
    }
    for (int phase : {1, 2}) {
        prepare(true);
        const auto before = g_weapon;
        g_throw_at = phase;
        StormboltTransitionAmmoAdapter<2> adapter(g_store, api());
        bool threw = false;
        try { adapter.apply(g_weapon.data(), g_token); }
        catch (const std::runtime_error&) { threw = true; }
        assert(threw && before == g_weapon && g_calls.size() == static_cast<std::size_t>(phase));
        StormboltStageState state{};
        assert(g_store.snapshot(g_token, state) && state.counter == 7 && state.action_timer == 8.75f);

        prepare(true);
        const auto before_invalid = g_weapon;
        g_invalidate_at = phase;
        assert(adapter.apply(g_weapon.data(), g_token) == StormboltTransitionAmmoResult::invalidated_after_native);
        assert(before_invalid == g_weapon && g_calls.size() == static_cast<std::size_t>(phase));
        StormboltInstanceToken replacement{};
        assert(g_store.token_for_live_address(reinterpret_cast<std::uintptr_t>(g_weapon.data()), replacement));
        assert(g_store.snapshot(replacement, state) && state.counter == 0 && state.action_timer == 0);
    }
    prepare(true);
    StormboltTransitionAmmoAdapter<2> adapter(g_store, api());
    const auto before = g_weapon;
    assert(g_store.retire(g_token));
    assert(adapter.apply(g_weapon.data(), g_token) == StormboltTransitionAmmoResult::rejected_before_start);
    assert(g_calls.empty() && before == g_weapon);
    prepare(true);
    StormboltTransitionAmmoAdapter<2> incomplete(g_store, {});
    assert(incomplete.apply(g_weapon.data(), g_token) == StormboltTransitionAmmoResult::rejected_before_start);
    auto wrong = g_token;
    ++wrong.address;
    assert(adapter.apply(g_weapon.data(), wrong) == StormboltTransitionAmmoResult::rejected_before_start);
    assert(g_calls.empty() && get<std::uint8_t>(g_weapon.data(), 0x2350) == 0x6D);
    std::printf("Transition ammo fixture passed: %u gate/owner combinations, raw timer bits, native exceptions and stale generations; no game calls.\n", combinations);
}
