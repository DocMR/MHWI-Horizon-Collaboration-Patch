#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_special_to_normal_hook.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes = std::array<std::byte, N>;
Bytes<0x30> g_action{};
std::array<Bytes<0xE488>, 3> g_actors{};
std::array<Bytes<0x2860>, 3> g_weapons{};
std::array<Bytes<0x1610>, 3> g_parameters{};
std::array<Bytes<0x400>, 3> g_components{};
std::array<Bytes<0x1AA0>, 3> g_banks{};
std::array<Bytes<0x2D0>, 3> g_events{};
Bytes<16> g_manager{}, g_character{}, g_resource{}, g_result{}, g_dti{}, g_context{};
void* g_manager_slot = g_manager.data();
void* g_provider_slot = g_manager.data();
StormboltInstanceStore<3> g_store;
std::array<StormboltInstanceToken, 3> g_tokens{};
std::array<bool, 3> g_active{};
std::vector<int> g_calls;
bool g_query_result = false, g_motion_swap = false, g_save_swap = false;
bool g_effect_swap = false, g_throw_create = false;
unsigned g_motion_count = 0, g_creates = 0, g_clears = 0, g_saves = 0;
void* g_query_actor = nullptr;
void* g_entry_component = nullptr;
void* g_create_owner = nullptr;

template<class T> void put(void* p, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(p)+offset, &value, sizeof(value));
}
template<class T> T get(const void* p, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(p)+offset, sizeof(value));
    return value;
}
std::size_t bank_index(const void* p) {
    for (std::size_t i = 0; i < 3; ++i) if (p == g_banks[i].data()+0x1A40) return i;
    assert(false); return 0;
}
std::size_t component_index(const void* p) {
    for (std::size_t i = 0; i < 3; ++i) if (p == g_components[i].data()) return i;
    assert(false); return 0;
}
void* construct(void*, void* parameter) { std::memset(parameter, 0, 0xF0); return parameter; }
void selector(void*, void*) { }
void* create(void*, void*, void* resource, std::uint32_t group, std::uint32_t record,
    void* parameter, void* owner, void* extra) {
    assert(resource == g_resource.data() && group == 0x7EE && record == 0 && !extra);
    assert(get<void*>(parameter, 0x70) == owner);
    g_create_owner = owner;
    g_calls.push_back(6); ++g_creates;
    if (g_throw_create) throw std::runtime_error("native effect");
    if (g_effect_swap) put(g_action.data(), 0x28, g_actors[2].data());
    return g_result.data();
}
std::uint8_t post_needed(void*) { return 0; }
void post(void*, void*, const void*, std::uint32_t) { assert(false); }
bool active(const void* bank) { g_calls.push_back(5); return g_active[bank_index(bank)]; }
void* assign(void* bank, void* result) {
    assert(result == g_result.data());
    g_calls.push_back(7); g_active[bank_index(bank)] = true; return bank;
}
void clear(void* component) {
    g_calls.push_back(8); ++g_clears; g_active[component_index(component)] = false;
    if (g_effect_swap) put(g_action.data(), 0x28, g_actors[2].data());
}
void* character(void* manager) { assert(manager == g_manager.data()); g_calls.push_back(3); return g_character.data(); }
void save_bit(void* target, std::int32_t bit) {
    assert(target == g_character.data() && bit == 11);
    g_calls.push_back(4); ++g_saves;
    // Locks must not be held across native callbacks.
    StormboltStageState state{};
    assert(g_store.snapshot(g_tokens[0], state));
    if (g_save_swap) put(g_action.data(), 0x28, g_actors[1].data());
}
bool motion(void* actor, std::uint32_t packed, std::uint32_t id, std::uint32_t mode,
    std::uint32_t extra, float start, float rate) {
    const auto layer = g_motion_count++;
    assert(layer < 3 && packed == 0x200u+layer && id == 0xC0FCu-layer);
    assert(mode == (layer == 0 ? 2u : layer == 1 ? 1u : 3u));
    assert(extra == 0 && start == -1.0f && rate == 2.0f);
    assert(actor == g_actors[g_motion_swap ? layer : 0].data());
    g_calls.push_back(10+static_cast<int>(layer));
    if (g_motion_swap && layer < 2) put(g_action.data(), 0x28, g_actors[layer+1].data());
    return false; // Native gate rejection must not stop subsequent requests.
}
void actor_flag(void* actor, std::uint32_t flag) {
    assert(actor == g_actors[0].data() && (flag == 0x5D || flag == 0x28));
    g_calls.push_back(flag == 0x5D ? 1 : 2);
    for (std::size_t base : {0x8938u, 0x8968u}) {
        const auto off = base+(flag/32)*4;
        put(actor, off, get<std::uint32_t>(actor, off) | (1u << (flag%32)));
    }
}
void* event(void* actor) {
    assert(actor == g_actors[0].data());
    return g_events[0].data();
}
void weapon_effect(void* component, std::uint32_t group, std::uint32_t record, std::uint32_t param) {
    assert(group == 0x7D0 && record == 0 && param == 0xFFFFFFFF);
    g_calls.push_back(13); g_entry_component = component;
}
bool query(void* actor, std::uint32_t channel) {
    assert(channel == 0); g_calls.push_back(9); g_query_actor = actor; return g_query_result;
}
StormboltPlayerEffectApi effects_api() {
    return {g_dti.data(), construct, selector, &g_provider_slot, g_context.data(),
        create, post_needed, &g_manager_slot, post, active, assign, clear};
}
StormboltSpecialToNormalApi action_api() { return {motion, actor_flag, event, weapon_effect, query}; }
void prepare() {
    g_calls.clear(); g_active.fill(false); g_action.fill(std::byte{0});
    g_motion_count = g_creates = g_clears = g_saves = 0;
    g_query_result = g_motion_swap = g_save_swap = g_effect_swap = g_throw_create = false;
    g_query_actor = g_entry_component = g_create_owner = nullptr;
    for (std::size_t i = 0; i < 3; ++i) {
        g_actors[i].fill(std::byte{0}); g_weapons[i].fill(std::byte{0x6D});
        g_events[i].fill(std::byte{0}); g_components[i].fill(std::byte{0});
        put(g_actors[i].data(), 0xC, std::uint32_t{0xE});
        put(g_weapons[i].data(), 0xC, std::uint32_t{0xE});
        put(g_weapons[i].data(), 0x9F8, std::uint32_t{13});
        put(g_weapons[i].data(), 0x9FC, std::uint32_t{i == 1 ? 238u : 239u});
        put(g_weapons[i].data(), 0x9B0, g_actors[i].data());
        put(g_actors[i].data(), 0x76B0, g_weapons[i].data());
        put(g_actors[i].data(), 0x7810, g_parameters[i].data());
        put(g_parameters[i].data(), 0x1608, 1.25f);
        put(g_actors[i].data(), 0x8808, g_components[i].data());
        put(g_actors[i].data(), 0x8938, std::uint32_t{1});
        put(g_components[i].data(), 8, g_banks[i].data());
        put(g_components[i].data(), 0x18, g_actors[i].data());
        put(g_components[i].data(), 0x30, g_resource.data());
        g_tokens[i] = g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapons[i].data()));
        assert(g_tokens[i]);
    }
    put(g_action.data(), 0x28, g_actors[0].data());
}
}

int main() {
    StormboltTransitionSaveApi save{&g_manager_slot, character, save_bit};
    StormboltTransitionAmmoAdapter<3> ammo(g_store, save);
    StormboltPlayerEffectAdapter effects(effects_api());
    StormboltSpecialToNormalAdapter<3> action(g_store, ammo, effects, action_api());
    prepare(); g_motion_swap = true;
    assert(action.enter(g_action.data()) == StormboltSpecialActionStatus::processed);
    assert(g_calls == std::vector<int>({10, 11, 12, 13}));
    assert(g_entry_component == g_components[2].data());

    unsigned combinations = 0;
    for (bool trigger : {false, true}) for (bool model_gate : {false, true})
        for (bool enabled : {false, true}) for (bool already_active : {false, true})
        for (bool complete : {false, true}) {
        prepare(); g_active[0] = already_active; g_query_result = complete;
        put(g_events[0].data(), 0x190, std::uint32_t{trigger ? 0xA7u : 0xA5u});
        put(g_events[0].data(), 0x178, (model_gate ? 0x08000000u : 0u) |
            (enabled ? 0x10000000u : 0u) | 0x55u);
        auto expected = g_weapons[0];
        auto flags = get<std::uint32_t>(expected.data(), 0x2D0);
        put(expected.data(), 0x2D0, model_gate ? flags & ~0x400u : flags | 0x400u);
        if (trigger) {
            put(expected.data(), 0x2350, std::uint8_t{1});
            put(expected.data(), 0x27C8, std::uint32_t{40});
        }
        const auto result = action.update(g_action.data());
        assert(result.status == StormboltSpecialActionStatus::processed && result.animation_complete == complete);
        assert(expected == g_weapons[0]);
        assert(get<std::uint32_t>(g_events[0].data(), 0x190) == 0xA5);
        assert(g_saves == (trigger ? 1u : 0u));
        assert(g_creates == (enabled && !already_active ? 1u : 0u));
        assert(g_clears == (enabled ? 0u : 1u));
        assert(g_query_actor == g_actors[0].data());
        assert(g_calls.front() == 1 && g_calls[1] == 2 && g_calls.back() == 9);
        assert(action.exit(g_action.data()) == StormboltSpecialActionStatus::processed);
        assert(get<std::uint32_t>(g_weapons[0].data(), 0x2D0) == (flags | 0x400u));
        ++combinations;
    }
    // Save switches the action's actor. Retain event0, mutate weapon1's model,
    // and let effect creation switch to actor2 before the final query.
    prepare(); g_save_swap = g_effect_swap = true;
    put(g_events[0].data(), 0x190, std::uint32_t{2});
    put(g_events[0].data(), 0x178, std::uint32_t{0x18000000});
    const auto second_flags = get<std::uint32_t>(g_weapons[1].data(), 0x2D0);
    const auto switched = action.update(g_action.data());
    assert(switched.status == StormboltSpecialActionStatus::processed);
    assert(get<std::uint32_t>(g_weapons[0].data(), 0x27C8) == 40);
    assert(get<std::uint32_t>(g_weapons[1].data(), 0x27C8) == 0x6D6D6D6D);
    assert(get<std::uint32_t>(g_weapons[1].data(), 0x2D0) == (second_flags & ~0x400u));
    assert(g_create_owner == g_actors[1].data() && g_query_actor == g_actors[2].data());
    assert(g_active[1] && !g_active[0] && g_calls == std::vector<int>({1,2,3,4,5,6,7,9}));

    // Enable uses weapon owner; disable uses action actor, even if they differ.
    prepare(); put(g_weapons[0].data(), 0x9B0, g_actors[1].data());
    put(g_events[0].data(), 0x178, std::uint32_t{0x10000000});
    assert(action.update(g_action.data()).status == StormboltSpecialActionStatus::processed);
    assert(g_create_owner == g_actors[1].data() && g_active[1]);
    put(g_events[0].data(), 0x178, std::uint32_t{0}); g_active[0] = true;
    assert(action.update(g_action.data()).status == StormboltSpecialActionStatus::processed);
    assert(!g_active[0] && g_active[1]);
    prepare(); put(g_weapons[0].data(), 0x9B0, static_cast<void*>(nullptr));
    put(g_events[0].data(), 0x178, std::uint32_t{0x10000000});
    assert(action.update(g_action.data()).status == StormboltSpecialActionStatus::processed);
    assert(g_creates == 0 && g_query_actor == g_actors[0].data());

    prepare(); g_throw_create = true;
    put(g_events[0].data(), 0x190, std::uint32_t{2});
    put(g_events[0].data(), 0x178, std::uint32_t{0x10000000});
    bool threw = false;
    try { action.update(g_action.data()); } catch (const std::runtime_error&) { threw = true; }
    assert(threw && g_saves == 1 && !g_query_actor && !g_active[0]);
    assert(get<std::uint32_t>(g_events[0].data(), 0x190) == 0); // No replay/fallback.
    prepare(); assert(g_store.retire(g_tokens[0]));
    assert(action.enter(g_action.data()) == StormboltSpecialActionStatus::unavailable);
    assert(action.update(g_action.data()).status == StormboltSpecialActionStatus::unavailable);
    assert(action.exit(g_action.data()) == StormboltSpecialActionStatus::unavailable && g_calls.empty());
    prepare(); put(g_weapons[0].data(), 0x9FC, std::uint32_t{237});
    assert(action.update(g_action.data()).status == StormboltSpecialActionStatus::unavailable && g_calls.empty());
    prepare();
    auto missing_provider_api = effects_api();
    missing_provider_api.provider_resolver_context = g_context.data();
    missing_provider_api.resolve_provider = +[](const void*, StormboltEffectProviderSnapshot&) {
        g_calls.push_back(14); return false;
    };
    StormboltPlayerEffectAdapter missing_provider(missing_provider_api);
    StormboltSpecialToNormalAdapter<3> missing_action(g_store, ammo, missing_provider, action_api());
    put(g_events[0].data(), 0x190, std::uint32_t{2});
    put(g_events[0].data(), 0x178, std::uint32_t{0x10000000});
    assert(missing_action.update(g_action.data()).status == StormboltSpecialActionStatus::invalidated);
    assert(g_calls == std::vector<int>({1,2,3,4,5,14}));
    assert(g_saves == 1 && !g_creates && !g_query_actor && !g_active[0]);
    assert(get<std::uint32_t>(g_events[0].data(), 0x190) == 0); // No replay after consumption.
    // Exercise the real three-slot dispatcher with the real candidate core and
    // existing ammo/effect adapters. Only native game APIs are fixture callbacks.
    prepare();
    std::array<std::uintptr_t, 9> action_table{};
    std::array<std::uintptr_t, 1> actor_table{}, weapon_table{};
    const auto boundary = +[](void*) { assert(false); };
    const auto original_update = +[](void*) -> bool { assert(false); return false; };
    action_table[6] = action_table[8] = reinterpret_cast<std::uintptr_t>(boundary);
    action_table[7] = reinterpret_cast<std::uintptr_t>(original_update);
    const auto table_before = action_table;
    put(g_action.data(), 0, action_table.data());
    for (std::size_t i = 0; i < 3; ++i) {
        put(g_actors[i].data(), 0, actor_table.data());
        put(g_weapons[i].data(), 0, weapon_table.data());
    }
    StormboltSpecialToNormalHook<3> hook(g_store, action);
    const StormboltSpecialToNormalBindings binding{
        reinterpret_cast<std::uintptr_t>(action_table.data()),
        reinterpret_cast<std::uintptr_t>(actor_table.data()),
        reinterpret_cast<std::uintptr_t>(weapon_table.data()), boundary, boundary, original_update,
        +[](void* value, StormboltSpecialActionPhase) {
            assert(value == g_action.data());
            return true; // All fixture storage lives until after restore.
        }};
    assert(hook.install(binding));
    put(g_events[0].data(), 0x190, std::uint32_t{2});
    put(g_events[0].data(), 0x178, std::uint32_t{0x10000000});
    g_query_result = true;
    reinterpret_cast<StormboltActionBoundary>(action_table[6])(g_action.data());
    assert(reinterpret_cast<StormboltActionUpdate>(action_table[7])(g_action.data()));
    reinterpret_cast<StormboltActionBoundary>(action_table[8])(g_action.data());
    assert(g_calls == std::vector<int>({10,11,12,13,1,2,3,4,5,6,7,9}));
    assert(get<std::uint32_t>(g_weapons[0].data(), 0x27C8) == 40);
    assert(get<std::uint32_t>(g_weapons[0].data(), 0x2D0) & 0x400);
    assert(g_saves == 1 && g_creates == 1 && g_active[0]);
    assert(hook.restore() && !hook.active() && action_table == table_before);
    std::printf("Special-to-normal candidate passed: entry/update/exit, %u branch combinations, live rereads, original-event retention and native exception propagation; no game activation.\n", combinations);
}
