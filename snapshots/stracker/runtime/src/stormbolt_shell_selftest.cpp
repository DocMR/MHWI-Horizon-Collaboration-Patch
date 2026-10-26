#include "stormbolt_shell_adapter.h"
#include <array>
#include <cassert>
#include <iostream>

using namespace stormslinger;
namespace {
struct Record { std::uint64_t untouched = 0xabcdef; void* descriptor = nullptr; };
static_assert(sizeof(Record) == 16 && offsetof(Record, descriptor) == 8);
struct Fixture {
    StormboltInstanceStore<2> store;
    int weapon = 0, second_weapon = 0, owner = 0, shell = 0;
    std::array<int, 4> descriptors{};
    std::array<Record, 98> records{};
    std::array<unsigned char, 0xC0> context{};
    StormboltInstanceToken token{}, second{};
    unsigned reads = 0, creates = 0;
    std::uint32_t seen_selector = 0;
    void* expected_weapon = nullptr;
    bool missing = false, fail_create = false;
};
Fixture* active = nullptr;
const void* record_at(void* list, std::uint32_t selector) {
    auto& f = *active;
    assert(list == f.records.data());
    ++f.reads;
    f.seen_selector = selector;
    // The adapter must not hold the sidecar lock while entering native code.
    StormboltStageState state{};
    assert(f.store.snapshot(f.token, state));
    return !f.missing && selector < f.records.size() ? &f.records[selector] : nullptr;
}
void* create_wrapped(void* descriptor, void* weapon, void* owner, const void* context) {
    auto& f = *active;
    ++f.creates;
    assert(descriptor == f.records[f.seen_selector].descriptor);
    assert(weapon == f.expected_weapon && owner == &f.owner && context == f.context.data());
    assert(f.context.front() == 0xa5 && f.context.back() == 0xa5);
    StormboltStageState state{};
    assert(f.store.snapshot(f.token, state));
    return f.fail_create ? nullptr : &f.shell;
}
void set_snapshot(Fixture& f, StormboltInstanceToken token, std::uint32_t stage) {
    assert(f.store.transact(token, [stage](auto& state) noexcept {
        state.shot_snapshot = stage;
        state.counter = 123; // Selection must use snapshot, not counter.
        return true;
    }));
}
}
int main() {
    Fixture f;
    active = &f;
    f.context.fill(0xa5);
    f.token = f.store.initialize(reinterpret_cast<std::uintptr_t>(&f.weapon));
    f.second = f.store.initialize(reinterpret_cast<std::uintptr_t>(&f.second_weapon));
    for (unsigned n = 0; n != 4; ++n) f.records[94 + n].descriptor = &f.descriptors[n];
    StormboltBorrowedShot shot{&f.weapon, f.records.data(), &f.owner, f.context.data(), 13, 239, 40};
    const StormboltShellCalls calls{record_at, create_wrapped};
    f.expected_weapon = &f.weapon;
    const auto records_before = f.records;
    const auto context_before = f.context;
    const unsigned expected[] = {94,94,94,95,95,96,96,96,97,97};
    for (unsigned stage = 0; stage != 10; ++stage) {
        set_snapshot(f, f.token, stage);
        auto result = create_stormbolt_shell(f.store, f.token, shot, calls);
        assert(result.status == StormboltShellStatus::created && result.shell == &f.shell);
        assert(result.selector == expected[stage] && f.seen_selector == expected[stage]);
        StormboltStageState state{};
        assert(f.store.snapshot(f.token, state) && state.shot_snapshot == stage && state.counter == 123);
    }
    set_snapshot(f, f.token, UINT32_MAX);
    assert(create_stormbolt_shell(f.store, f.token, shot, calls).selector == 97);
    assert(f.context == context_before);
    for (std::size_t i = 0; i < f.records.size(); ++i)
        assert(f.records[i].descriptor == records_before[i].descriptor
            && f.records[i].untouched == records_before[i].untouched);

    // Independent weapons share the immutable native list but not the stage.
    auto other = shot;
    other.weapon = &f.second_weapon;
    other.equipment_id = 238;
    f.expected_weapon = &f.second_weapon;
    set_snapshot(f, f.second, 3);
    assert(create_stormbolt_shell(f.store, f.second, other, calls).selector == 95);
    f.expected_weapon = &f.weapon;
    assert(create_stormbolt_shell(f.store, f.token, shot, calls).selector == 97);

    const auto reads_before = f.reads, creates_before = f.creates;
    for (unsigned n = 0; n < 3; ++n) {
        auto unrelated = shot;
        if (n == 0) unrelated.category = 12;
        if (n == 1) unrelated.equipment_id = 237;
        if (n == 2) unrelated.ammo = 39;
        assert(create_stormbolt_shell(f.store, f.token, unrelated, calls).status
            == StormboltShellStatus::unrelated);
    }
    for (unsigned n = 0; n < 6; ++n) {
        auto invalid = shot;
        auto missing_calls = calls;
        if (n == 0) invalid.shell_list = nullptr;
        if (n == 1) invalid.owner = nullptr;
        if (n == 2) invalid.context = nullptr;
        if (n == 3) invalid.weapon = &f.second_weapon;
        if (n == 4) missing_calls.record_at = nullptr;
        if (n == 5) missing_calls.create_wrapped = nullptr;
        assert(create_stormbolt_shell(f.store, f.token, invalid, missing_calls).status
            == StormboltShellStatus::invalid_binding);
    }
    const auto old = f.token;
    f.token = f.store.initialize(old.address);
    assert(create_stormbolt_shell(f.store, old, shot, calls).status
        == StormboltShellStatus::stale_instance);
    assert(f.reads == reads_before && f.creates == creates_before);
    f.missing = true;
    assert(create_stormbolt_shell(f.store, f.token, shot, calls).status
        == StormboltShellStatus::missing_record);
    f.missing = false;
    f.records[94].descriptor = nullptr;
    assert(create_stormbolt_shell(f.store, f.token, shot, calls).status
        == StormboltShellStatus::missing_descriptor);
    assert(f.creates == creates_before);
    f.records[94].descriptor = &f.descriptors[0];
    f.fail_create = true;
    auto result = create_stormbolt_shell(f.store, f.token, shot, calls);
    assert(result.status == StormboltShellStatus::creation_failed && !result.shell);
    assert(f.creates == creates_before + 1); // Never retry or silently spawn a fallback.
    assert(f.store.retire(f.token));
    assert(create_stormbolt_shell(f.store, f.token, shot, calls).status
        == StormboltShellStatus::stale_instance);
    assert(f.creates == creates_before + 1);
    std::cout << "PASS: shell stages, independent instances, stale lifetimes, native argument forwarding, "
                 "callback reentry, immutable list/context and failure propagation\n";
}
