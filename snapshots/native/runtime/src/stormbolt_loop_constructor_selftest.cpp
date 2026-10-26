#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_lifecycle_hooks.h"
#include "stormbolt_power_cell_loop_state.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
StormboltPowerCellLoopStore<4> store;
StormboltLifecycleHooks<4,StormboltPowerCellLoopState,StormboltPowerCellLoopToken> bridge(store);
struct Object { unsigned initialized=0; } objects[2];
unsigned constructions=0,destructions=0;
StormboltPowerCellLoopToken old{};
__declspec(noinline) void* construct(void* p) {
    StormboltPowerCellLoopState state{};
    if (old) assert(!store.snapshot(old,state));
    ++constructions;
    if (p) static_cast<Object*>(p)->initialized=123;
    return p;
}
__declspec(noinline) void* destroy(void* p,std::uint32_t flags) {
    StormboltPowerCellLoopToken token{};
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(p),token));
    assert(flags==5);
    ++destructions;
    static_cast<Object*>(p)->initialized=0;
    return p;
}
}
int main() {
    HookTransaction transaction;
    assert(transaction.begin()==MH_OK);
    assert(bridge.add_constructor_to_transaction(transaction,
        reinterpret_cast<void*>(&construct),reinterpret_cast<void*>(&destroy))==MH_OK);
    assert(transaction.commit()==MH_OK);
    for (auto& object:objects) {
        assert(construct(&object)==&object && object.initialized==123);
        StormboltPowerCellLoopToken token{};
        assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),token));
    }
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&objects[0]),old));
    assert(store.transact(old,[](auto& state) noexcept { state={5.0f,true};return true; }));
    assert(construct(&objects[0])==&objects[0]);
    StormboltPowerCellLoopToken fresh{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&objects[0]),fresh));
    StormboltPowerCellLoopState state{};
    assert(fresh.generation!=old.generation && store.snapshot(fresh,state));
    assert(state.timer==0.0f && !state.active && state.visual_timer==-1.0f &&
           !state.visual_active && !state.growth && state.visual_phase==0);
    for (auto& object:objects) assert(destroy(&object,5)==&object);
    assert(constructions==3 && destructions==2);
    assert(transaction.stop()==MH_OK);
    std::puts("PASS: shared ctor loop state, two elements, retire-before-reconstruct/destroy and native returns");
}
