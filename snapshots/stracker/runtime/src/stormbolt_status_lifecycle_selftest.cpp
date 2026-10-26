#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_status_lifecycle_hooks.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
StormboltPowerCellStore<2> store;
struct Object {std::uintptr_t vtable=0;void* owner=nullptr;unsigned resets=0;} object,other;
unsigned constructions=0,destructions=0;
bool expected_preserve=false;
StormboltPowerCellToken prior{};
__declspec(noinline) void reset(void* p,bool preserve_effects) {
    assert(preserve_effects==expected_preserve);
    if(preserve_effects) return;
    StormboltPowerCellState state{};
    if (prior) assert(!store.snapshot(prior,state));
    ++static_cast<Object*>(p)->resets;
}
__declspec(noinline) void* construct(void* p,void* owner) {
    StormboltPowerCellState state{};
    if (prior) assert(!store.snapshot(prior,state));
    ++constructions;
    auto& o=*static_cast<Object*>(p);o.vtable=0x1234;o.owner=owner;
    reset(p,false);return p;
}
__declspec(noinline) void destroy(void* p) {
    StormboltPowerCellToken token{};
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(p),token));
    ++destructions;static_cast<Object*>(p)->vtable=0;
}
}
int main() {
    HookTransaction transaction;assert(transaction.begin()==MH_OK);
    StormboltStatusLifecycleHooks<2> bridge(store,0x1234);
    assert(bridge.add_to_transaction(transaction,reinterpret_cast<void*>(&construct),
        reinterpret_cast<void*>(&reset),reinterpret_cast<void*>(&destroy))==MH_OK);
    assert(transaction.commit()==MH_OK);
    int owner=0;
    assert(construct(&object,&owner)==&object && object.owner==&owner && object.resets==1);
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),prior));
    assert(store.transact(prior,[](auto& s) noexcept {s={3,9.0f,true};return true;}));
    expected_preserve=true;reset(&object,true);expected_preserve=false;
    StormboltPowerCellState preserved{};
    assert(store.snapshot(prior,preserved) && preserved.count==3 && preserved.timer==9.0f && preserved.event_eligible);
    reset(&object,false);
    StormboltPowerCellToken current{};StormboltPowerCellState state{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),current));
    assert(current.generation!=prior.generation && store.snapshot(current,state));
    assert(state.count==0 && state.timer==-1.0f && !state.event_eligible && object.resets==2);
    prior=current;
    assert(construct(&object,&owner)==&object && object.resets==3);
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),current));
    assert(current.generation!=prior.generation);
    prior=current;destroy(&object);
    assert(!store.snapshot(current,state) && destructions==1);
    prior={};other.vtable=0x1234;reset(&other,false);
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&other),current));
    assert(transaction.stop()==MH_OK && bridge.release_after_unhook());
    assert(construct(&object,&owner)==&object);
    assert(!store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),current));
    std::puts("PASS: status ctor owner ABI, nested reset, generation reset/reuse, inner destructor, original restoration");
}
