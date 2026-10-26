#ifdef NDEBUG
#undef NDEBUG // Selftests must execute assertions in Release builds too.
#endif
#include "stormbolt_lifecycle_hooks.h"
#include "stormbolt_power_cell_loop_state.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace stormslinger;
namespace {
#ifdef STORMBOLT_LOOP_LIFECYCLE_TEST
using TestState=StormboltPowerCellLoopState;
using TestToken=StormboltPowerCellLoopToken;
#else
using TestState=StormboltStageState;
using TestToken=StormboltInstanceToken;
#endif
StormboltInstanceStore<2,TestState,TestToken> store;
StormboltLifecycleHooks<2,TestState,TestToken> bridge(store);
auto& test_value(StormboltStageState& state) { return state.counter; }
auto& test_value(StormboltPowerCellLoopState& state) { return state.timer; }
#ifdef STORMBOLT_LOOP_LIFECYCLE_TEST
constexpr float initial_value=0.0f;
#else
constexpr unsigned initial_value=0;
#endif
struct Object { std::uint64_t initialized = 0; } first, second, overflow;
Object* allocation_result = &first;
void* last_type = nullptr;
std::uint32_t last_flags = 0;
unsigned allocate_calls = 0, place_calls = 0, destroy_calls = 0;
bool validate_retirement = false, validate_placement_retirement=false;
TestToken placement_retired{};
TestToken expected_retired{};
__declspec(noinline) void* native_allocate(void* type) {
    ++allocate_calls;
    last_type = type;
    if (allocation_result) allocation_result->initialized = 0x1234;
    return allocation_result;
}
__declspec(noinline) void* native_place(void* type, void* storage) {
    ++place_calls;
    last_type = type;
    if(validate_placement_retirement) {
        TestState state{};TestToken token{};
        assert(!store.snapshot(placement_retired,state));
        assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(storage),token));
    }
    if (storage) static_cast<Object*>(storage)->initialized = 0x5678;
    return storage;
}
unsigned array_calls=0;
__declspec(noinline) void* native_array_place(void* type,void* storage,std::uint32_t count) {
    ++array_calls;last_type=type;
    auto* rows=static_cast<Object*>(storage);
    for(std::uint32_t n=0;n<count;++n) rows[n].initialized=0x9ABC;
    return storage;
}
__declspec(noinline) void* native_destroy(void* object, std::uint32_t flags) {
    ++destroy_calls;
    last_flags = flags;
    if (validate_retirement) {
        TestState state{};
        assert(!store.snapshot(expected_retired, state));
        TestToken token{};
        assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(object), token));
    }
    static_cast<Object*>(object)->initialized = 0;
    return object;
}
TestToken token_for(void* object) {
    TestToken token{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(object), token));
    return token;
}
}
int main() {
    StormboltLifecycleEndpoints targets{reinterpret_cast<void*>(&native_allocate),
        reinterpret_cast<void*>(&native_place), reinterpret_cast<void*>(&native_destroy),
        reinterpret_cast<void*>(&native_array_place),sizeof(Object)};
    std::array<std::array<unsigned char, 32>, 4> before{};
    const void* addresses[] = {targets.allocate, targets.place, targets.destroy, targets.array_place};
    for (unsigned i = 0; i < 4; ++i) std::memcpy(before[i].data(), addresses[i], 32);
    HookTransaction transaction;
    assert(transaction.begin() == MH_OK);
    assert(bridge.add_to_transaction(transaction, targets) == MH_OK);
    int type = 0;
    assert(native_allocate(&type) == &first && allocate_calls == 1);
    TestToken missing{123, 456};
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&first), missing));
    assert(missing.address == 123 && missing.generation == 456);
    assert(transaction.commit() == MH_OK);
    assert(native_allocate(&type) == &first && first.initialized == 0x1234 && last_type == &type);
    auto one = token_for(&first);
    assert(store.transact(one, [](auto& state) noexcept { test_value(state) = 7; return true; }));
    assert(native_place(&type, &second) == &second && second.initialized == 0x5678);
    auto two = token_for(&second);
    TestState state{};
    assert(store.snapshot(two, state) && test_value(state) == initial_value);
    allocation_result = nullptr;
    assert(native_allocate(&type) == nullptr);
    assert(bridge.diagnostics.constructed == 2);
    allocation_result = &overflow;
    assert(native_allocate(&type) == &overflow && overflow.initialized == 0x1234);
    assert(bridge.diagnostics.tracking_failures == 1);
    assert(store.snapshot(one, state) && test_value(state) == 7); // Capacity cannot evict live state.
    assert(native_place(&type, nullptr) == nullptr);
    assert(bridge.diagnostics.constructed == 2);

    validate_retirement = true;
    expected_retired = one;
    assert(native_destroy(&first, 5) == &first && last_flags == 5 && !first.initialized);
    assert(bridge.diagnostics.retired == 1);
    assert(native_place(&type, &first) == &first);
    const auto recreated = token_for(&first);
    assert(recreated.generation != one.generation);
    assert(!store.snapshot(one, state) && store.snapshot(recreated, state) && test_value(state) == initial_value);
    expected_retired = recreated;
    assert(native_destroy(&first, 0) == &first && last_flags == 0);
    expected_retired = two;
    assert(native_destroy(&second, 1) == &second && last_flags == 1);
    validate_retirement = false;
    assert(native_destroy(&overflow, 1) == &overflow);
    assert(bridge.diagnostics.untracked_destructions == 1);
    assert(bridge.diagnostics.constructed == 3 && bridge.diagnostics.retired == 3);

    // Test from INSIDE native placement, before the new fields are written.
    assert(native_place(&type,&first)==&first);
    placement_retired=token_for(&first);
    assert(store.transact(placement_retired,[](auto& value) noexcept {test_value(value)=7;return true;}));
    validate_placement_retirement=true;
    assert(native_place(&type,&first)==&first);
    validate_placement_retirement=false;
    const auto placed_again=token_for(&first);
    assert(placed_again.generation!=placement_retired.generation);
    assert(!store.snapshot(placement_retired,state));
    assert(store.snapshot(placed_again,state) && test_value(state)==initial_value);
    assert(native_destroy(&first,0)==&first);

    Object batch[2]{};
    assert(native_array_place(&type,batch,0)==batch && array_calls==1);
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(batch),missing));
    assert(native_array_place(&type,batch,2)==batch && array_calls==2 && last_type==&type);
    const auto batch_one=token_for(batch),batch_two=token_for(batch+1);
    assert(batch[0].initialized==0x9ABC && batch[1].initialized==0x9ABC);
    assert(store.transact(batch_one,[](auto& value) noexcept {test_value(value)=7;return true;}));
    assert(native_array_place(&type,batch,2)==batch && array_calls==3);
    const auto replaced_one=token_for(batch),replaced_two=token_for(batch+1);
    assert(replaced_one.generation!=batch_one.generation && replaced_two.generation!=batch_two.generation);
    assert(!store.snapshot(batch_one,state) && !store.snapshot(batch_two,state));
    assert(store.snapshot(replaced_one,state) && test_value(state)==initial_value);
    assert(native_destroy(batch,0)==batch && native_destroy(batch+1,0)==batch+1);

    // No worker threads or callbacks remain at this quiescent test boundary.
    assert(transaction.stop() == MH_OK);
    for (unsigned i = 0; i < 4; ++i) assert(!std::memcmp(before[i].data(), addresses[i], 32));
    allocation_result = &first;
    assert(native_allocate(&type) == &first);
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&first), missing));
    assert(allocate_calls == 5 && place_calls == 5 && destroy_calls == 7);
    std::puts("PASS: actual queued MinHook lifecycle callbacks, both constructors, native failures, "
              "retire-before-destruction, address reuse, preserved ABI and byte-exact restore");
}
