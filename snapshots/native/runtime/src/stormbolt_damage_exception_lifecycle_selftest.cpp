#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_lifecycle_hooks.h"
#include "stormbolt_damage_exception_store.h"
#include "stormbolt_damage_exception_hooks.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
StormboltDamageExceptionStore<4> store;
StormboltLifecycleHooks<4,StormboltDamageExceptionState,StormboltDamageExceptionToken> bridge(store);
struct Object { std::uintptr_t initialized=0; } objects[2];
StormboltDamageExceptionHooks<4> updates(store,123);
std::array<std::byte,0x7660> actor{};
std::array<std::byte,0xDB8> absorption{};
unsigned absorbs=0,update_calls=0;
__declspec(noinline) float absorb(void*,float input) {++absorbs;return input*0.25f;}
__declspec(noinline) void update(void* object) {
    StormboltDamageExceptionToken token{};StormboltDamageExceptionState state{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(object),token));
    assert(store.snapshot(token,state) && state.absorbed_positive_input);
    ++update_calls;
}
unsigned constructions=0,destructions=0;
StormboltDamageExceptionToken old{};
__declspec(noinline) void* construct(void* p) {
    StormboltDamageExceptionState state{};
    if (old) assert(!store.snapshot(old,state));
    ++constructions;
    if (p) static_cast<Object*>(p)->initialized=123;
    return p;
}
__declspec(noinline) void* destroy(void* p,std::uint32_t flags) {
    StormboltDamageExceptionToken token{};
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
    assert(updates.add_to_transaction(transaction,reinterpret_cast<void*>(&update),
        reinterpret_cast<void*>(&absorb))==MH_OK);
    assert(transaction.commit()==MH_OK);
    for (auto& object:objects) {
        assert(construct(&object)==&object && object.initialized==123);
        StormboltDamageExceptionToken token{};
        assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&object),token));
    }
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&objects[0]),old));
    assert(store.transact(old,[](auto& state) noexcept { state.absorbed_positive_input=true;return true; }));
    assert(construct(&objects[0])==&objects[0]);
    StormboltDamageExceptionToken fresh{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(&objects[0]),fresh));
    StormboltDamageExceptionState state{};
    assert(fresh.generation!=old.generation && store.snapshot(fresh,state));
    assert(!state.absorbed_positive_input);
    void* actor_pointer=actor.data();void* component_pointer=&objects[0];
    std::memcpy(absorption.data()+0xDB0,&actor_pointer,sizeof(actor_pointer));
    std::memcpy(actor.data()+0x7658,&component_pointer,sizeof(component_pointer));
    assert(absorb(absorption.data(),8.0f)==2.0f && absorbs==1);
    assert(store.snapshot(fresh,state) && state.absorbed_positive_input);
    update(&objects[0]);
    assert(update_calls==1 && store.snapshot(fresh,state) && !state.absorbed_positive_input);
    for (auto& object:objects) assert(destroy(&object,5)==&object);
    assert(constructions==3 && destructions==2);
    assert(transaction.stop()==MH_OK);
    assert(updates.release_after_unhook());
    assert(absorb(absorption.data(),4.0f)==1.0f && absorbs==2);
    std::puts("PASS: damage exception ctor state, independent instances, retire-before-reconstruct/destroy and native returns");
}
