#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_coefficient_hook.h"
#include "stormbolt_runtime_dispatch_gate.h"
#include <cassert>
#include <cstdio>

using namespace stormslinger;
extern "C" {
void stormbolt_test_coefficient_call(void*, std::uint32_t, void*);
extern char stormbolt_test_coefficient_return;
}
namespace {
StormboltInstanceStore<2> registry;
StormboltCoefficientHook<2> coefficient_hook(registry);
template<std::size_t N> struct Object { std::array<unsigned char, N> bytes{}; };
Object<0x2860> weapon, second_weapon;
Object<0x7820> actor;
Object<0x176C> parameters;
Object<0x6B0> shell_info;
std::array<std::uintptr_t, 2> info_table{}, param_table{};
StormboltRuntimeDispatchGate publication;
bool dispatch_ready() {return publication.weapon();}
unsigned original_calls = 0;
void* forwarded_weapon = nullptr;
void* forwarded_info = nullptr;
std::uint32_t forwarded_ammo = 0;
bool original_overwrites = false;
StormboltInstanceToken callback_token{};
template<class T, class Obj> void field(Obj& object, std::size_t offset, const T& value) {
    assert(offset + sizeof(value) <= object.bytes.size());
    std::memcpy(object.bytes.data() + offset, &value, sizeof(value));
}
__declspec(noinline) void original_callback(void* receiver, std::uint32_t ammo, void* info) {
    ++original_calls;
    forwarded_weapon = receiver; forwarded_info = info; forwarded_ammo = ammo;
    StormboltStageState state{};
    assert(registry.snapshot(callback_token, state)); // Callback outside registry lock.
    if (original_overwrites) {
        const float value = 33.5f;
        std::memcpy(static_cast<unsigned char*>(info) + 0x60, &value, sizeof(value));
    }
}
void initialize_info(void* associated_weapon = &weapon) {
    shell_info.bytes.fill(0xA5);
    field(shell_info, 0, info_table.data());
    field(shell_info, 0x80, associated_weapon);
}
float value_at(std::size_t offset) {
    float value = 0;
    std::memcpy(&value, shell_info.bytes.data() + offset, sizeof(value));
    return value;
}
void stage(StormboltInstanceToken token, std::uint32_t value) {
    assert(registry.transact(token, [value](auto& state) noexcept {
        state.shot_snapshot = value; state.counter = 7; return true;
    }));
}
void call(void* receiver = &weapon, std::uint32_t ammo = 40, void* info = &shell_info) {
    const auto before = original_calls;
    stormbolt_test_coefficient_call(receiver, ammo, info);
    assert(original_calls == before + 1);
    assert(forwarded_weapon == receiver && forwarded_ammo == ammo && forwarded_info == info);
}
}
int main() {
    auto* table = static_cast<std::uintptr_t*>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(table);
    for (unsigned i = 0; i < 124; ++i) table[i] = reinterpret_cast<std::uintptr_t>(&original_callback);
    const auto table_copy = [&] {
        std::array<std::uintptr_t, 124> copy{};
        std::memcpy(copy.data(), table, sizeof(copy)); return copy;
    }();
    DWORD old = 0;
    assert(VirtualProtect(table, 4096, PAGE_READONLY, &old));
    for (auto* target : {&weapon, &second_weapon}) {
        field(*target, 0, table);
        field(*target, 0xC, std::uint8_t{2});
        field(*target, 0x9F8, std::uint32_t{13});
        field(*target, 0x9FC, target == &weapon ? std::uint32_t{238} : std::uint32_t{239});
        field(*target, 0x9B0, &actor);
        field(*target, 0x2388, std::uint32_t{1});
    }
    field(actor, 0xC, std::uint8_t{2});
    field(actor, 0x7810, &parameters);
    field(parameters, 0, param_table.data());
    for (unsigned i = 0; i < 9; ++i) {
        std::memcpy(parameters.bytes.data() + 0x1608 + 40 * i,
            ps_stormbolt_rows[i].fields.data(), 7 * sizeof(float));
        field(parameters, 0x1624 + 40 * i, ps_stormbolt_rows[i].flag);
    }
    const auto one = registry.initialize(reinterpret_cast<std::uintptr_t>(&weapon));
    const auto two = registry.initialize(reinterpret_cast<std::uintptr_t>(&second_weapon));
    callback_token = one;
    std::array<unsigned char, 32> shared_callback_code{};
    std::memcpy(shared_callback_code.data(), reinterpret_cast<const void*>(&original_callback), shared_callback_code.size());
    StormboltCoefficientBindings binding{table + 123, &original_callback,
        reinterpret_cast<std::uintptr_t>(&stormbolt_test_coefficient_return),
        reinterpret_cast<std::uintptr_t>(table), reinterpret_cast<std::uintptr_t>(info_table.data()),
        reinterpret_cast<std::uintptr_t>(param_table.data()), &dispatch_ready};
    assert(coefficient_hook.install(binding) && coefficient_hook.active());
    stage(one, UINT32_MAX); initialize_info();
    const auto dormant_info = shell_info.bytes;
    call(); assert(shell_info.bytes == dormant_info);
    StormboltStageState dormant_stage{};
    assert(registry.snapshot(one, dormant_stage) && dormant_stage.shot_snapshot == UINT32_MAX);
    publication.enable();

    MEMORY_BASIC_INFORMATION memory{};
    assert(VirtualQuery(table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
    for (unsigned i = 0; i < 123; ++i) assert(table[i] == table_copy[i]);
    assert(!std::memcmp(shared_callback_code.data(), reinterpret_cast<const void*>(&original_callback), shared_callback_code.size()));
    constexpr std::array<std::uint32_t, 11> stages{0,1,2,3,4,5,6,7,8,9,UINT32_MAX};
    for (auto index : stages) {
        initialize_info();
        auto before = shell_info.bytes;
        stage(one, index);
        call();
        const auto clamped = (std::min)(index, std::uint32_t{8});
        assert(value_at(0x60) == ps_stormbolt_rows[clamped].fields[5]);
        assert(value_at(0x6C) == ps_stormbolt_rows[clamped].fields[6]);
        std::memcpy(before.data() + 0x60, shell_info.bytes.data() + 0x60, 4);
        std::memcpy(before.data() + 0x6C, shell_info.bytes.data() + 0x6C, 4);
        assert(before == shell_info.bytes); // Exactly two DWORD writes.
        StormboltStageState state{};
        assert(registry.snapshot(one, state) && state.shot_snapshot == clamped && state.counter == 7);
    }
    stage(two, 3); initialize_info(&second_weapon); call(&second_weapon);
    assert(value_at(0x6C) == ps_stormbolt_rows[3].fields[6]);
    stage(one, 8); initialize_info();
    field(parameters, 0x161C + 8 * 40, 17.25f);
    call(); assert(value_at(0x60) == 17.25f); // Live table, not generated default.
    original_overwrites = true;
    call(); assert(value_at(0x60) == 33.5f); // Original callback happens after our write.
    original_overwrites = false;

    auto rejected = [&] {
        const auto before = shell_info.bytes;
        const auto applied = coefficient_hook.diagnostics.applied.load();
        call();
        assert(shell_info.bytes == before && coefficient_hook.diagnostics.applied == applied);
    };
    for(auto* target:{&weapon,&second_weapon}) {
        const auto token=target==&weapon?one:two;
        field(*target,0x2388,std::uint32_t{0});
        stage(token,UINT32_MAX);initialize_info(target);
        const auto native_info=shell_info.bytes;
        const auto calls_before=original_calls;
        call(target);
        assert(original_calls==calls_before+1 && shell_info.bytes==native_info);
        StormboltStageState preserved{};
        assert(registry.snapshot(token,preserved) && preserved.shot_snapshot==UINT32_MAX);
        field(*target,0x2388,std::uint32_t{1});
    }
    initialize_info();
    const auto before = shell_info.bytes;
    call(&weapon, 39); assert(shell_info.bytes == before);
    reinterpret_cast<StormboltCoefficientCallback>(table[123])(&weapon, 40, &shell_info);
    assert(shell_info.bytes == before); // Wrong callsite.
    reinterpret_cast<StormboltCoefficientCallback>(table[122])(&weapon, 40, &shell_info);
    assert(shell_info.bytes == before); // Another slot sharing the original function.
    field(weapon, 0x9FC, std::uint32_t{237}); rejected();
    field(weapon, 0x9FC, std::uint32_t{238});
    field(shell_info, 0x80, &second_weapon); rejected(); field(shell_info, 0x80, &weapon);
    field(shell_info, 0, param_table.data()); rejected(); field(shell_info, 0, info_table.data());
    field(actor, 0xC, std::uint8_t{0}); rejected(); field(actor, 0xC, std::uint8_t{2});
    // Native deactivation leaves the object tracked until deferred destruction.
    stage(one, UINT32_MAX);
    field(weapon, 0xC, std::uint8_t{0x10}); rejected();
    StormboltStageState inactive_stage{};
    assert(registry.snapshot(one,inactive_stage) && inactive_stage.shot_snapshot==UINT32_MAX);
    field(weapon, 0xC, std::uint8_t{2});
    field(parameters, 0, info_table.data()); rejected(); field(parameters, 0, param_table.data());
    field(parameters, 0x161C + 8 * 40, (std::numeric_limits<float>::quiet_NaN)());
    stage(one, UINT32_MAX); rejected();
    StormboltStageState unchanged{};
    assert(registry.snapshot(one, unchanged) && unchanged.shot_snapshot == UINT32_MAX);
    field(parameters, 0x161C + 8 * 40, 17.25f);
    assert(registry.retire(two)); initialize_info(&second_weapon);
    const auto prior = shell_info.bytes; call(&second_weapon); assert(shell_info.bytes == prior);

    auto* readonly_info = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(readonly_info);
    initialize_info(); std::memcpy(readonly_info, shell_info.bytes.data(), shell_info.bytes.size());
    assert(VirtualProtect(readonly_info, 4096, PAGE_READONLY, &old));
    call(&weapon, 40, readonly_info);
    assert(!std::memcmp(readonly_info, shell_info.bytes.data(), shell_info.bytes.size()));
    assert(VirtualFree(readonly_info, 0, MEM_RELEASE));


    publication.begin_restore();
    for (unsigned phase = 0; phase < 2; ++phase) {
        stage(one, UINT32_MAX); initialize_info();
        const auto disabled_info = shell_info.bytes;
        call(); assert(shell_info.bytes == disabled_info);
        StormboltStageState disabled_stage{};
        assert(registry.snapshot(one, disabled_stage) && disabled_stage.shot_snapshot == UINT32_MAX);
        publication.finish_restore();
    }
    std::puts("PASS coefficient publication: dormant/restoring/stopped preserve info/state and one original callback.");
    assert(coefficient_hook.restore() && !coefficient_hook.active());
    assert(!std::memcmp(table, table_copy.data(), sizeof(table_copy)));
    assert(!std::memcmp(shared_callback_code.data(), reinterpret_cast<const void*>(&original_callback), shared_callback_code.size()));
    assert(VirtualQuery(table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
    initialize_info(); const auto restored = shell_info.bytes; call(); assert(shell_info.bytes == restored);
    assert(VirtualFree(table, 0, MEM_RELEASE));
    std::puts("PASS: scoped virtual callback, live parameter rows, stage clamp, two-field writes, "
              "native callback order/reentry, read-only rejection and exact vtable restoration");
}
