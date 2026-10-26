#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_special_to_normal_hook.h"
#include <cassert>
#include <cstdio>
#include <vector>

template class stormslinger::StormboltSpecialToNormalHook<2>;
using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes = std::array<std::byte, N>;
Bytes<0x30> g_action{};
Bytes<0x76B8> g_actor{};
Bytes<0xA00> g_weapon{};
std::array<std::uintptr_t, 1> g_actor_table{}, g_weapon_table{};
StormboltInstanceStore<2> g_store;
StormboltInstanceToken g_token{};
unsigned g_original_enter = 0, g_original_update = 0, g_original_exit = 0, g_ready_calls = 0;
bool g_ready = true, g_throw_ready = false;
std::vector<StormboltSpecialActionPhase> g_phases;
template<class T> void put(void* p, std::size_t o, T v) {
    std::memcpy(static_cast<std::byte*>(p)+o, &v, sizeof(v));
}
void original_enter(void*) { ++g_original_enter; }
bool original_update(void*) { ++g_original_update; return true; }
void original_exit(void*) { ++g_original_exit; }
void foreign_callback(void*) { }
bool ready(void* action, StormboltSpecialActionPhase phase) {
    assert(action == g_action.data()); ++g_ready_calls; g_phases.push_back(phase);
    StormboltStageState state{};
    assert(g_store.snapshot(g_token, state)); // Not called under sidecar lock.
    if (g_throw_ready) throw 81;
    return g_ready;
}
struct Core {
    unsigned enters = 0, updates = 0, exits = 0;
    bool throws = false, complete = true;
    StormboltSpecialActionStatus status = StormboltSpecialActionStatus::processed;
    StormboltSpecialActionStatus enter(void*) { ++enters; if (throws) throw 82; return status; }
    StormboltSpecialActionResult update(void*) { ++updates; if (throws) throw 82; return {status,complete}; }
    StormboltSpecialActionStatus exit(void*) { ++exits; if (throws) throw 82; return status; }
};
struct Protect {
    inline static unsigned calls = 0, fail_at = 0;
    inline static std::vector<std::uintptr_t> writes;
    static BOOL protect(void* p, SIZE_T bytes, DWORD flags, DWORD* old) {
        ++calls;
        if (calls == fail_at) return FALSE;
        if (flags == PAGE_READWRITE) writes.push_back(reinterpret_cast<std::uintptr_t>(p));
        return VirtualProtect(p, bytes, flags, old);
    }
};
using Hook = StormboltSpecialToNormalHook<2,Core,OwnedVtablePointer<Protect>>;
void set_slot(std::uintptr_t* table, unsigned index, std::uintptr_t value) {
    DWORD old = 0, unused = 0;
    assert(VirtualProtect(table, 4096, PAGE_READWRITE, &old));
    table[index] = value;
    assert(VirtualProtect(table, 4096, old, &unused));
}
}
int main() {
    auto* table = static_cast<std::uintptr_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(table);
    table[6] = reinterpret_cast<std::uintptr_t>(&original_enter);
    table[7] = reinterpret_cast<std::uintptr_t>(&original_update);
    table[8] = reinterpret_cast<std::uintptr_t>(&original_exit);
    std::array<std::uintptr_t,9> before{};
    std::memcpy(before.data(),table,sizeof(before));
    std::array<unsigned char,16> original_code{};
    std::memcpy(original_code.data(),reinterpret_cast<void*>(&original_enter),original_code.size());
    DWORD old = 0;
    assert(VirtualProtect(table,4096,PAGE_READONLY,&old));
    put(g_action.data(),0,table); put(g_action.data(),0x28,g_actor.data());
    put(g_actor.data(),0,g_actor_table.data()); put(g_actor.data(),0xC,std::uint8_t{2});
    put(g_actor.data(),0x76B0,g_weapon.data()); put(g_weapon.data(),0,g_weapon_table.data());
    put(g_weapon.data(),0xC,std::uint8_t{2}); put(g_weapon.data(),0x9B0,g_actor.data());
    put(g_weapon.data(),0x9F8,std::uint32_t{13}); put(g_weapon.data(),0x9FC,std::uint32_t{238});
    g_token = g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data())); assert(g_token);
    const StormboltSpecialToNormalBindings binding{reinterpret_cast<std::uintptr_t>(table),
        reinterpret_cast<std::uintptr_t>(g_actor_table.data()),reinterpret_cast<std::uintptr_t>(g_weapon_table.data()),
        original_enter,original_exit,original_update,ready};
    Core core;
    Hook hook(g_store,core);
    auto invalid = binding; invalid.ready = nullptr;
    assert(!hook.install(invalid) && !hook.active());
    invalid = binding; invalid.action_vtable += 1;
    assert(!hook.install(invalid) && !hook.active());
    assert(hook.install(binding) && hook.active());
    assert(!hook.install(binding));
    const auto enter = reinterpret_cast<StormboltActionBoundary>(table[6]);
    const auto update = reinterpret_cast<StormboltActionUpdate>(table[7]);
    const auto leave = reinterpret_cast<StormboltActionBoundary>(table[8]);
    enter(g_action.data()); assert(update(g_action.data())); leave(g_action.data());
    assert(core.enters==1 && core.updates==1 && core.exits==1);
    assert(g_phases == std::vector<StormboltSpecialActionPhase>({StormboltSpecialActionPhase::enter,
        StormboltSpecialActionPhase::update,StormboltSpecialActionPhase::exit}));
    assert(!g_original_enter && !g_original_update && !g_original_exit);
    put(g_weapon.data(),0x9FC,std::uint32_t{239}); assert(update(g_action.data()));
    const auto forward_all = [&](void* action) {
        const auto e=g_original_enter,u=g_original_update,x=g_original_exit;
        enter(action); assert(update(action)); leave(action);
        assert(g_original_enter==e+1 && g_original_update==u+1 && g_original_exit==x+1);
    };
    forward_all(nullptr);
    put(g_weapon.data(),0x9FC,std::uint32_t{237}); forward_all(g_action.data());
    put(g_weapon.data(),0x9FC,std::uint32_t{238});
    put(g_weapon.data(),0x9F8,std::uint32_t{12}); forward_all(g_action.data());
    put(g_weapon.data(),0x9F8,std::uint32_t{13});
    put(g_weapon.data(),0x9B0,static_cast<void*>(nullptr)); forward_all(g_action.data());
    put(g_weapon.data(),0x9B0,g_actor.data());
    put(g_actor.data(),0,g_weapon_table.data()); forward_all(g_action.data());
    put(g_actor.data(),0,g_actor_table.data());
    put(g_weapon.data(),0,g_actor_table.data()); forward_all(g_action.data());
    put(g_weapon.data(),0,g_weapon_table.data());
    put(g_action.data(),0,g_actor_table.data()); forward_all(g_action.data());
    put(g_action.data(),0,table);
    assert(g_store.retire(g_token)); forward_all(g_action.data());
    g_token = g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data()));
    g_ready=false; forward_all(g_action.data()); g_ready=true;
    const auto original_updates=g_original_update;
    core.status=StormboltSpecialActionStatus::invalidated;
    assert(!update(g_action.data()) && g_original_update==original_updates);
    core.status=StormboltSpecialActionStatus::unavailable;
    assert(!update(g_action.data()) && g_original_update==original_updates);
    core.status=StormboltSpecialActionStatus::processed; core.complete=false;
    assert(!update(g_action.data()) && g_original_update==original_updates);
    core.throws=true;
    bool threw=false;
    try { update(g_action.data()); } catch(int n) { threw=n==82; }
    assert(threw && g_original_update==original_updates); core.throws=false;
    g_throw_ready=true; threw=false;
    try { update(g_action.data()); } catch(int n) { threw=n==81; }
    assert(threw && g_original_update==original_updates); g_throw_ready=false;
    Protect::writes.clear();
    assert(hook.restore() && !hook.active());
    assert(Protect::writes == std::vector<std::uintptr_t>({binding.action_vtable+0x40,
        binding.action_vtable+0x38,binding.action_vtable+0x30}));
    assert(!std::memcmp(table,before.data(),sizeof(before)));

    // Each write/protection-restoration failure during three-slot installation.
    for(unsigned failure=1;failure<=6;++failure) {
        Core next_core; Hook next(g_store,next_core);
        Protect::calls=0; Protect::fail_at=failure;
        assert(!next.install(binding));
        Protect::fail_at=0;
        assert(next.restore() && !next.active());
        assert(!std::memcmp(table,before.data(),sizeof(before)));
        MEMORY_BASIC_INFORMATION memory{};
        assert(VirtualQuery(table,&memory,sizeof(memory)) && memory.Protect==PAGE_READONLY);
    }
    // Preserve foreign slot values and stop reverse teardown at the first fault.
    for(unsigned index=6;index<=8;++index) {
        Core next_core; Hook next(g_store,next_core);
        assert(next.install(binding));
        const auto replacement=table[index];
        set_slot(table,index,reinterpret_cast<std::uintptr_t>(&foreign_callback));
        assert(!next.restore() && next.active());
        assert(table[index]==reinterpret_cast<std::uintptr_t>(&foreign_callback));
        for(unsigned i=6;i<index;++i) assert(table[i]!=before[i]);
        for(unsigned i=index+1;i<=8;++i) assert(table[i]==before[i]);
        set_slot(table,index,replacement);
        assert(next.restore() && !next.active());
        assert(!std::memcmp(table,before.data(),sizeof(before)));
    }
    for(unsigned index=6;index<=8;++index) {
        Core next_core; Hook next(g_store,next_core);
        set_slot(table,index,reinterpret_cast<std::uintptr_t>(&foreign_callback));
        assert(!next.install(binding) && !next.active());
        for(unsigned i=6;i<=8;++i) if(i!=index) assert(table[i]==before[i]);
        set_slot(table,index,before[index]);
    }
    assert(!std::memcmp(original_code.data(),reinterpret_cast<void*>(&original_enter),original_code.size()));
    assert(VirtualFree(table,0,MEM_RELEASE));
    std::puts("Special action hook passed: three scoped slots, target forwarding, no replay, reverse restore, six install faults and three restore conflicts; no game activation.");
}
