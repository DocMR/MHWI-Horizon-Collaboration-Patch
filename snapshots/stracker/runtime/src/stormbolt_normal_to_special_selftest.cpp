#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_normal_to_special_hook.h"
#include "stormbolt_runtime_dispatch_gate.h"
#include <cassert>
#include <cstdio>

using namespace stormslinger;
namespace {
StormboltRuntimeDispatchGate publication;
bool dispatch_ready() {return publication.weapon();}
StormboltInstanceStore<2> g_store;
StormboltNormalToSpecialHook<2> g_hook(g_store);
template<std::size_t N> struct Object { std::array<unsigned char, N> bytes{}; };
Object<0x30> g_action;
Object<0xE478> g_actor;
Object<0x2860> g_weapon;
std::array<std::uintptr_t, 0x2C8/8> g_actor_table{};
std::array<std::uintptr_t, 1> g_weapon_table{};
unsigned g_original_calls = 0, g_getter_calls = 0, g_query_calls = 0;
bool g_query_result = false, g_query_throws = false;
std::uint32_t g_expected_flags = 0;
void* g_expected_event = nullptr;
StormboltInstanceToken g_token{};
template<class T, class Obj> void set(Obj& object, std::size_t offset, T value) {
    assert(offset + sizeof(T) <= object.bytes.size());
    std::memcpy(object.bytes.data() + offset, &value, sizeof(T));
}
template<class T> T get(void* object, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<unsigned char*>(object) + offset, sizeof(T));
    return value;
}
__declspec(noinline) bool original(void* action) {
    assert(action == &g_action || action == nullptr);
    ++g_original_calls;
    return true; // Deliberately differs from native false to prove forwarding.
}
void* event_object(void* actor) {
    assert(actor == &g_actor);
    ++g_getter_calls;
    StormboltStageState state{};
    assert(g_store.snapshot(g_token, state)); // No held sidecar lock.
    return get<void*>(actor, 0xE470);
}
bool query(void* actor, std::uint32_t channel) {
    assert(actor == &g_actor && channel == 0);
    assert(get<std::uint32_t>(g_expected_event, 0x28) == g_expected_flags);
    ++g_query_calls;
    StormboltStageState state{};
    assert(g_store.snapshot(g_token, state));
    if (g_query_throws) throw 71;
    return g_query_result;
}
bool call() {
    const auto* table = get<std::uintptr_t*>(&g_action, 0);
    return reinterpret_cast<StormboltActionUpdate>(table[7])(&g_action);
}
}

int main() {
    auto* table = static_cast<std::uintptr_t*>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    auto* event = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(table && event);
    for (unsigned i = 0; i < 10; ++i) table[i] = reinterpret_cast<std::uintptr_t>(&original);
    std::array<std::uintptr_t, 10> table_before{};
    std::memcpy(table_before.data(), table, sizeof(table_before));
    std::array<unsigned char, 16> code_before{};
    std::memcpy(code_before.data(), reinterpret_cast<void*>(&original), code_before.size());
    DWORD previous = 0;
    assert(VirtualProtect(table, 4096, PAGE_READONLY, &previous));
    set(g_action, 0, table); set(g_action, 0x28, &g_actor);
    set(g_actor, 0, g_actor_table.data()); set(g_actor, 0xC, std::uint8_t{2});
    set(g_actor, 0x76B0, &g_weapon); set(g_actor, 0xE470, event);
    g_actor_table[0x2C0/8] = reinterpret_cast<std::uintptr_t>(&query);
    set(g_weapon, 0, g_weapon_table.data()); set(g_weapon, 0xC, std::uint8_t{2});
    set(g_weapon, 0x9B0, &g_actor); set(g_weapon, 0x9F8, std::uint32_t{13});
    set(g_weapon, 0x9FC, std::uint32_t{238});
    g_token = g_store.initialize(reinterpret_cast<std::uintptr_t>(&g_weapon));
    assert(g_token);
    StormboltNormalToSpecialBindings bindings{table+7, &original,
        reinterpret_cast<std::uintptr_t>(table), reinterpret_cast<std::uintptr_t>(g_actor_table.data()),
        reinterpret_cast<std::uintptr_t>(g_weapon_table.data()), &event_object, &query, &dispatch_ready};
    auto invalid = bindings; invalid.slot = table+6;
    assert(!g_hook.install(invalid));
    assert(g_hook.install(bindings) && g_hook.active());
    for (unsigned i = 0; i < 10; ++i) if (i != 7) assert(table[i] == table_before[i]);
    MEMORY_BASIC_INFORMATION memory{};
    assert(VirtualQuery(table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
    g_expected_event = event;
    auto blocked_publication = [&] {
        std::memset(event, 0xA5, 4096);
        std::array<unsigned char, 4096> before{};
        std::memcpy(before.data(), event, before.size());
        const auto original_before = g_original_calls, getter_before = g_getter_calls, query_before = g_query_calls;
        assert(call());
        assert(g_original_calls == original_before + 1 && g_getter_calls == getter_before && g_query_calls == query_before);
        assert(!std::memcmp(before.data(), event, before.size()));
    };
    blocked_publication();
    publication.enable();
    const auto enabled_original_baseline = g_original_calls;

    for (const auto id : {238u, 239u}) {
        set(g_weapon, 0x9FC, id);
        for (const auto flags : {0u, 1u, 2u, 0x80000001u, 0xFFFFFFFFu}) {
            for (const auto result : {false, true}) {
                std::memset(event, 0xA5, 4096);
                std::memcpy(event+0x28, &flags, 4);
                std::array<unsigned char, 4096> expected{};
                std::memcpy(expected.data(), event, expected.size());
                g_expected_flags = flags & ~1u;
                std::memcpy(expected.data()+0x28, &g_expected_flags, 4);
                g_query_result = result;
                const auto before = g_query_calls;
                assert(call() == result && g_query_calls == before+1);
                assert(std::memcmp(expected.data(), event, expected.size()) == 0);
                assert(g_original_calls == enabled_original_baseline);
            }
        }
    }
    auto rejected = [&] {
        std::array<unsigned char, 4096> before{};
        std::memcpy(before.data(), event, before.size());
        const auto prior_queries = g_query_calls, prior_original = g_original_calls;
        assert(call());
        assert(g_query_calls == prior_queries && g_original_calls == prior_original+1);
        assert(!std::memcmp(before.data(), event, before.size()));
    };
    set(g_weapon, 0x9FC, std::uint32_t{237}); rejected();
    set(g_weapon, 0x9FC, std::uint32_t{238});
    set(g_weapon, 0x9F8, std::uint32_t{12}); rejected(); set(g_weapon, 0x9F8, std::uint32_t{13});
    set(g_weapon, 0x9B0, std::uintptr_t{0}); rejected(); set(g_weapon, 0x9B0, &g_actor);
    set(g_weapon, 0xC, std::uint8_t{0}); rejected(); set(g_weapon, 0xC, std::uint8_t{2});
    set(g_actor, 0xC, std::uint8_t{0}); rejected(); set(g_actor, 0xC, std::uint8_t{2});
    g_actor_table[0x2C0/8] = 0; rejected(); g_actor_table[0x2C0/8] = reinterpret_cast<std::uintptr_t>(&query);
    set(g_actor, 0, g_weapon_table.data()); rejected(); set(g_actor, 0, g_actor_table.data());
    set(g_actor, 0xE470, std::uintptr_t{0}); rejected(); set(g_actor, 0xE470, event);
    set(g_action, 0x28, std::uintptr_t{0}); rejected(); set(g_action, 0x28, &g_actor);
    assert(g_store.retire(g_token)); rejected();
    StormboltInstanceToken missing{};
    assert(!g_store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&g_weapon), missing));
    g_token = g_store.initialize(reinterpret_cast<std::uintptr_t>(&g_weapon));

    std::uint32_t flags = 1;
    std::memcpy(event+0x28, &flags, 4);
    assert(VirtualProtect(event, 4096, PAGE_READONLY, &previous)); rejected();
    assert(VirtualProtect(event, 4096, PAGE_READWRITE, &previous));
    flags = 2; std::memcpy(event+0x28, &flags, 4); g_expected_flags = flags;
    assert(VirtualProtect(event, 4096, PAGE_READONLY, &previous));
    const auto query_before = g_query_calls;
    g_query_result = false;
    assert(!call() && g_query_calls == query_before+1); // No write needed.
    assert(VirtualProtect(event, 4096, PAGE_READWRITE, &previous));
    flags = 3; std::memcpy(event+0x28, &flags, 4); g_expected_flags = 2;
    g_query_throws = true;
    const auto original_before = g_original_calls;
    bool caught = false;
    try { call(); } catch (int value) { assert(value == 71); caught = true; }
    assert(caught && g_original_calls == original_before && get<std::uint32_t>(event, 0x28) == 2);
    g_query_throws = false;
    assert(reinterpret_cast<StormboltActionUpdate>(table[6])(&g_action));

    publication.begin_restore(); blocked_publication();
    publication.finish_restore(); blocked_publication();
    std::puts("PASS normal-to-special publication: dormant/restoring/stopped preserve all event bytes, skip getter/query and forward one original call.");
    assert(g_hook.restore() && !g_hook.active());
    assert(!std::memcmp(table, table_before.data(), sizeof(table_before)));
    assert(!std::memcmp(reinterpret_cast<void*>(&original), code_before.data(), code_before.size()));
    assert(VirtualQuery(table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
    assert(call());
    assert(VirtualFree(event, 0, MEM_RELEASE));
    assert(VirtualFree(table, 0, MEM_RELEASE));
    std::puts("PASS: normal-to-special scoped update, 20 event/return cases, original fallback, "
              "no lazy state, native exception propagation and exact restoration");
}
