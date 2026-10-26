#ifdef NDEBUG
#undef NDEBUG // Selftests must execute assertions in Release builds too.
#endif
#include "stormbolt_shell_hook.h"
#include "stormbolt_runtime_dispatch_gate.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace stormslinger;
extern "C" {
void* stormbolt_test_shoot(void*, void*, void*, const void*, void*, std::uint32_t) noexcept(false);
extern char stormbolt_test_shoot_return;
int stormbolt_test_registers_intact = 0;
}
namespace {
StormboltRuntimeDispatchGate publication;
bool dispatch_ready() {return publication.weapon();}
StormboltInstanceStore<2> test_store;
StormboltShellHook<2> test_hook(test_store);
struct Weapon { std::array<unsigned char, 0x2860> bytes{}; } weapon_one, weapon_two;
struct Actor {std::array<unsigned char,0x7818> bytes{};} energy_actor;
struct Parameter {std::array<unsigned char,0x1770> bytes{};} energy_parameter;
std::uintptr_t parameter_vtable=0;
struct List { std::array<unsigned char, 0xB8> bytes{}; } shell_list;
struct Record { std::uint64_t unused; void* descriptor; };
static_assert(sizeof(Record) == 16);
std::array<Record, 98> records{};
std::array<std::uint64_t, 98> descriptors{};
std::array<std::uintptr_t, 18> list_vtable{};
std::array<std::uintptr_t, 2> weapon_vtable{};
StormboltInstanceToken reentry_token{};
bool throw_lookup = false, throw_create = false, null_result = false;
bool self_damage_seen=false;
bool shot_effect_seen=false;
float expected_post_energy=0;
unsigned self_damage_calls=0;
unsigned lookup_calls = 0, creation_calls = 0, post_creation = 0;
std::array<const void*, 4> forwarded{};
int native_shell = 0, owner_object = 0, context_object = 0;

template<class T, class Object>
void field(Object& object, std::size_t offset, const T& value) {
    assert(offset + sizeof(value) <= object.bytes.size());
    std::memcpy(object.bytes.data() + offset, &value, sizeof(value));
}
__declspec(noinline) const void* native_record_at(void* list, std::uint32_t index) {
    ++lookup_calls;
    assert(list == &shell_list);
    StormboltStageState copied{};
    assert(test_store.snapshot(reentry_token, copied)); // Must not hold registry lock.
    if (throw_lookup) throw std::runtime_error("lookup test");
    std::uint32_t count = 0;
    Record* base = nullptr;
    assert(read_stormbolt_field(reinterpret_cast<std::uintptr_t>(list), 0xB0, count));
    assert(read_stormbolt_field(reinterpret_cast<std::uintptr_t>(list), 0xA8, base));
    return index < count ? base + index : nullptr;
}
void initialize(Weapon& object, std::uint32_t id) {
    field(object, 0, weapon_vtable.data());
    field(object, 0xC, std::uint8_t{2});
    field(object, 0x9F8, std::uint32_t{13});
    field(object, 0x9FC, id);
    field(object, 0x2388, std::uint32_t{1});
    field(object, 0x1D90, &shell_list);
    field(object, 0x9B0, &energy_actor);
    // Deliberately differ from captured R14D. Re-reading mutable fields would
    // select the wrong ammo; the caller's actual register is authoritative.
    field(object, 0x27D0, std::uint32_t{0});
    field(object, 0x27C8, std::uint32_t{1});
}
void set_stage(StormboltInstanceToken token, std::uint32_t stage) {
    assert(test_store.transact(token, [stage](auto& value) noexcept {
        value.shot_snapshot = stage; return true;
    }));
}
bool before_energy(void* object,std::uint32_t snapshot) {
    assert(object==&weapon_one || object==&weapon_two);
    assert(snapshot<=8);self_damage_seen=true;++self_damage_calls;
    float gauge{};std::memcpy(&gauge,static_cast<Weapon*>(object)->bytes.data()+0x2358,4);
    expected_post_energy=(std::max)(0.0f,gauge-float(snapshot+1));
    return true;
}
bool after_energy(void* object,std::uint32_t snapshot) {
    assert(self_damage_seen && snapshot<=8);
    float gauge{};std::memcpy(&gauge,static_cast<Weapon*>(object)->bytes.data()+0x2358,4);
    assert(gauge==expected_post_energy);shot_effect_seen=true;return true;
}
void* shot(Weapon& object, std::uint32_t ammo = 40, void* supplied_descriptor = &descriptors[40],
           void* list = &shell_list, void* owner = &owner_object, const void* context = &context_object) {
    self_damage_seen=false;shot_effect_seen=false;
    auto* result = stormbolt_test_shoot(supplied_descriptor, &object, owner, context, list, ammo);
    ++post_creation;
    assert(stormbolt_test_registers_intact == 1);
    assert(forwarded[1] == &object && forwarded[2] == owner && forwarded[3] == context);
    return result;
}
}
extern "C" __declspec(noinline) void* stormbolt_test_create(
    void* descriptor, void* weapon, void* owner, const void* context) noexcept(false) {
    ++creation_calls;
    for(unsigned i=94;i<=97;++i)if(descriptor==&descriptors[i])assert(self_damage_seen && shot_effect_seen);
    forwarded = {descriptor, weapon, owner, context};
    if (throw_create) throw std::runtime_error("native test");
    return null_result ? nullptr : &native_shell;
}

int main() {
    field(energy_actor,0xC,std::uint8_t{0xE});
    field(energy_actor,0x7810,&energy_parameter);
    field(energy_parameter,0,&parameter_vtable);
    for(unsigned i=0;i<9;++i)field(energy_parameter,0x1610+40*i,float(i+1));
    for (std::size_t i = 0; i < records.size(); ++i) {
        descriptors[i] = i;
        records[i].descriptor = &descriptors[i];
    }
    list_vtable[0x88 / 8] = reinterpret_cast<std::uintptr_t>(&native_record_at);
    field(shell_list, 0, list_vtable.data());
    field(shell_list, 0xA8, records.data());
    field(shell_list, 0xB0, static_cast<std::uint32_t>(records.size()));
    initialize(weapon_one, 238);
    initialize(weapon_two, 239);
    const auto first = test_store.initialize(reinterpret_cast<std::uintptr_t>(&weapon_one));
    const auto second = test_store.initialize(reinterpret_cast<std::uintptr_t>(&weapon_two));
    reentry_token = first;
    const auto original_records = records;
    const auto original_list = shell_list.bytes;
    const auto original_weapon_one = weapon_one.bytes;
    const auto original_weapon_two = weapon_two.bytes;
    std::array<unsigned char, 32> original_code{};
    std::memcpy(original_code.data(), reinterpret_cast<const void*>(&stormbolt_test_create), original_code.size());

    HookTransaction transaction;
    assert(transaction.begin() == MH_OK);
    StormboltShellHookBindings bindings{reinterpret_cast<void*>(&stormbolt_test_create),
        reinterpret_cast<std::uintptr_t>(&stormbolt_test_shoot_return),
        reinterpret_cast<std::uintptr_t>(weapon_vtable.data()),
        reinterpret_cast<std::uintptr_t>(list_vtable.data()), &native_record_at, &dispatch_ready,
        reinterpret_cast<std::uintptr_t>(&parameter_vtable),&before_energy,&after_energy};
    assert(test_hook.add_to_transaction(transaction, bindings) == MH_OK);
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    assert(lookup_calls == 0 && test_hook.diagnostics.selected == 0);
    assert(transaction.commit() == MH_OK);
    set_stage(first, 8);
    const auto dormant_calls = creation_calls, dormant_lookups = lookup_calls;
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    assert(creation_calls == dormant_calls + 1 && lookup_calls == dormant_lookups);
    publication.enable();


    constexpr std::array<std::uint32_t, 11> stages{0,1,2,3,4,5,6,7,8,9,UINT32_MAX};
    constexpr std::array<std::uint32_t, 11> expected{94,94,94,95,95,96,96,96,97,97,97};
    for (std::size_t i = 0; i < stages.size(); ++i) {
        set_stage(first, stages[i]);
        const auto calls_before = creation_calls;
        assert(shot(weapon_one) == &native_shell);
        assert(forwarded[0] == &descriptors[expected[i]] && creation_calls == calls_before + 1);
        StormboltStageState state{};
        assert(test_store.snapshot(first, state) && state.shot_snapshot == (std::min)(stages[i],8u));
    }
    set_stage(second, 3);
    for(unsigned i=0;i<9;++i) {
        set_stage(first,i);field(weapon_one,0x2358,100.0f);
        assert(shot(weapon_one)==&native_shell);
        float energy{};std::memcpy(&energy,weapon_one.bytes.data()+0x2358,4);
        assert(energy==100.0f-float(i+1));
    }
    set_stage(first,8);field(weapon_one,0x2358,1.0f);
    assert(shot(weapon_one)==&native_shell);
    float depleted{};std::memcpy(&depleted,weapon_one.bytes.data()+0x2358,4);
    assert(depleted==0 && test_hook.diagnostics.energy_rejected==0);
    assert(shot(weapon_two) == &native_shell && forwarded[0] == &descriptors[95]);
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[97]);
    assert(shot(weapon_one, 40, &descriptors[40], &shell_list, nullptr, nullptr) == &native_shell);
    assert(forwarded[0] == &descriptors[97]); // Optional native arguments forwarded as-is.
    for(auto* target:{&weapon_one,&weapon_two}) {
        const auto token=target==&weapon_one?first:second;
        field(*target,0x2388,std::uint32_t{0});
        for(const auto stage:stages) {
            set_stage(token,stage);
            const auto calls=creation_calls,lookups=lookup_calls;
            assert(shot(*target)==&native_shell && forwarded[0]==&descriptors[40]);
            assert(creation_calls==calls+1 && lookup_calls==lookups);
            StormboltStageState preserved{};
            assert(test_store.snapshot(token,preserved) && preserved.shot_snapshot==stage);
        }
        field(*target,0x2388,std::uint32_t{1});
    }

    for(auto* target:{&weapon_one,&weapon_two}) {
        const auto token=target==&weapon_one?first:second;
        set_stage(token,UINT32_MAX);
        field(*target,0xC,std::uint8_t{0x10});
        const auto saved=target->bytes;
        const auto calls=creation_calls,lookups=lookup_calls;
        const auto energy=test_hook.diagnostics.energy_applied.load();
        const auto damage=self_damage_calls;
        assert(shot(*target)==&native_shell && forwarded[0]==&descriptors[40]);
        assert(creation_calls==calls+1 && lookup_calls==lookups);
        assert(target->bytes==saved && test_hook.diagnostics.energy_applied==energy && self_damage_calls==damage);
        StormboltStageState preserved{};
        assert(test_store.snapshot(token,preserved) && preserved.shot_snapshot==UINT32_MAX);
        field(*target,0xC,std::uint8_t{2});
        set_stage(token,8);
    }
    auto count_before = lookup_calls;
    assert(stormbolt_test_create(&descriptors[40], nullptr, nullptr, nullptr) == &native_shell);
    assert(lookup_calls == count_before && forwarded[0] == &descriptors[40]); // Other caller.
    assert(shot(weapon_one, 39) == &native_shell && forwarded[0] == &descriptors[40]);
    field(weapon_one, 0x9FC, std::uint32_t{237});
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    field(weapon_one, 0x9FC, std::uint32_t{238});
    field(weapon_one, 0x9F8, std::uint32_t{12});
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    field(weapon_one, 0x9F8, std::uint32_t{13});
    assert(shot(weapon_one, 40, &descriptors[40], nullptr) == &native_shell);
    assert(forwarded[0] == &descriptors[40]);
    assert(shot(weapon_one, 40, &descriptors[39]) == &native_shell && forwarded[0] == &descriptors[39]);

    records[97].descriptor = nullptr;
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    records[97].descriptor = &descriptors[97];
    field(shell_list, 0xB0, std::uint32_t{97});
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    field(shell_list, 0xB0, static_cast<std::uint32_t>(records.size()));
    throw_lookup = true;
    count_before = creation_calls;
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    assert(creation_calls == count_before + 1 && test_hook.diagnostics.selection_exceptions == 1);
    throw_lookup = false;
    assert(test_store.retire(second));
    count_before = lookup_calls;
    assert(shot(weapon_two) == &native_shell && forwarded[0] == &descriptors[40]);
    assert(lookup_calls == count_before && test_hook.diagnostics.untracked == 1);
    const auto recreated = test_store.initialize(reinterpret_cast<std::uintptr_t>(&weapon_two));
    assert(recreated.generation != second.generation);
    assert(shot(weapon_two) == &native_shell && forwarded[0] == &descriptors[94]);

    null_result = true;
    count_before = creation_calls;
    assert(shot(weapon_one) == nullptr && creation_calls == count_before + 1);
    assert(test_hook.diagnostics.native_null == 1); // Original null retained, no retry.
    null_result = false;
    throw_create = true;
    count_before = creation_calls;
    bool caught = false;
    try { shot(weapon_one); }
    catch (const std::runtime_error&) { caught = true; }
    assert(caught && creation_calls == count_before + 1); // Unwind through both MASM frames.
    throw_create = false;
    assert(shot(weapon_one) == &native_shell);


    publication.begin_restore();
    for (unsigned phase = 0; phase < 2; ++phase) {
        const auto native_before = creation_calls, lookup_before = lookup_calls;
        assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
        assert(creation_calls == native_before + 1 && lookup_calls == lookup_before);
        publication.finish_restore();
    }
    std::puts("PASS shell publication: dormant/restoring/stopped preserve descriptor and one native call; no resource lookup.");
    assert(!std::memcmp(records.data(), original_records.data(), sizeof(records)));
    assert(shell_list.bytes == original_list && weapon_one.bytes == original_weapon_one &&
        weapon_two.bytes == original_weapon_two);
    assert(post_creation + 2 == creation_calls); // One other caller, one unwound shot.
    assert(transaction.stop() == MH_OK); // All test callbacks quiescent.
    assert(!std::memcmp(original_code.data(), reinterpret_cast<const void*>(&stormbolt_test_create), original_code.size()));
    assert(shot(weapon_one) == &native_shell && forwarded[0] == &descriptors[40]);
    assert(self_damage_calls>0 && test_hook.diagnostics.self_damage_rejected==0);
    std::puts("PASS: native callsite self-damage callback before creation and energy cost for 9 live parameter rows, zero floor, snapshot clamp");
    std::puts("PASS: real x64 callsite/register capture, all stages, independent lifetimes, "
        "one original call, unchanged native continuation, rejection fallback, "
        "read-only resources, native returns/unwind and byte-exact removal");
}
