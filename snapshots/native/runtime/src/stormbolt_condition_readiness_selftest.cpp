#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_condition_readiness.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace stormslinger;
template<class T> void put(void* object,std::size_t offset,T value) {std::memcpy(static_cast<unsigned char*>(object)+offset,&value,sizeof(value));}
int main() {
    std::array<unsigned char,0x38> transition{};
    std::array<unsigned char,0x12610> actor{};
    std::array<unsigned char,0x2850> weapon{};
    std::array<unsigned char,0x1800> parameter{};
    std::array<unsigned char,0x1020> manager{};
    std::array<unsigned char,0x84> physical{};
    std::array<unsigned char,0xA8> component{};
    std::array<unsigned char,0x668> status{};
    std::array<unsigned char,0x40> authority{};
    std::array<unsigned char,0x146F0> system{};
    std::array<unsigned char,0xAE48> players{};
    std::array<unsigned char,0x10> other{};other[0xC]=2;
    auto* root=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x6000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(root);
    auto* noaccess=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(noaccess);
    put(transition.data(),0x10,actor.data());put(transition.data(),0x18,root);put(transition.data(),0x20,root+0x10);put(transition.data(),0x28,std::uint32_t{0x20});
    put(actor.data(),0,std::uintptr_t{1});actor[0xC]=2;put(actor.data(),0x76B0,weapon.data());
    put(actor.data(),0x14F8,root);put(actor.data(),0x12608,manager.data());put(actor.data(),0xE480,physical.data());
    put(actor.data(),0x7810,parameter.data());put(actor.data(),0x7658,component.data());put(actor.data(),0x7D20,status.data());
    put(weapon.data(),0,std::uintptr_t{2});weapon[0xC]=2;put(weapon.data(),0x9B0,actor.data());
    put(weapon.data(),0x9F8,std::uint32_t{13});put(weapon.data(),0x9FC,std::uint32_t{239});
    put(parameter.data(),0,std::uintptr_t{3});put(manager.data(),0,std::uintptr_t{4});
    put(manager.data(),0xD90,actor.data());put(manager.data(),0x40,actor.data());put(root,0x40,actor.data());
    put(players.data(),0xAE40,std::uint64_t{123});put(players.data(),0x48,std::uint64_t{123});put(players.data(),0x58,actor.data());
    void* authority_slot=authority.data();void* system_slot=system.data();void* player_slot=players.data();
    StormboltInstanceStore<1> store;auto token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));
    StormboltConditionReadinessBindings bindings{1,2,3,4,&authority_slot,&system_slot,&player_slot,false};
    StormboltConditionReadiness<1> unborrowed(store,bindings);
    assert(!unborrowed.shoot(transition.data(),weapon.data()) && !unborrowed.setbomb(transition.data(),weapon.data()));
    bindings.synchronous_borrow_verified=true;StormboltConditionReadiness<1> ready(store,bindings);
    auto both=[&] {return ready.shoot(transition.data(),weapon.data()) && ready.setbomb(transition.data(),weapon.data());};
    assert(both());
    bool dispatch=false;
    StormboltConditionReadiness<1> gated(store,bindings,+[](void* p){return *static_cast<bool*>(p);},&dispatch);
    assert(!gated.shoot(transition.data(),weapon.data()) && !gated.setbomb(transition.data(),weapon.data()));
    dispatch=true;
    assert(gated.shoot(transition.data(),weapon.data()) && gated.setbomb(transition.data(),weapon.data()));
    dispatch=false;
    assert(!gated.shoot(transition.data(),weapon.data()) && !gated.setbomb(transition.data(),weapon.data()));
    assert(!ready.shoot(noaccess,weapon.data()) && !ready.setbomb(transition.data(),noaccess));
    assert(!StormboltConditionReadiness<1>::ready_shoot(transition.data(),weapon.data(),nullptr));
    assert(StormboltConditionReadiness<1>::ready_setbomb(transition.data(),weapon.data(),&ready));
    put(root,0x40,other.data());assert(!both());put(root,0x40,actor.data());
    put(manager.data(),0xD90,other.data());assert(!both());put(manager.data(),0xD90,actor.data());
    put(weapon.data(),0x9B0,other.data());assert(!both());put(weapon.data(),0x9B0,actor.data());
    put(weapon.data(),0x9FC,std::uint32_t{237});assert(!both());put(weapon.data(),0x9FC,std::uint32_t{238});assert(both());
    put(actor.data(),0xE480,noaccess);assert(!both());put(actor.data(),0xE480,physical.data());
    put(actor.data(),0x7658,noaccess);assert(!ready.setbomb(transition.data(),weapon.data()));put(actor.data(),0x7658,component.data());
    system_slot=nullptr;assert(!ready.setbomb(transition.data(),weapon.data()));system_slot=system.data();
    authority_slot=noaccess;assert(!ready.setbomb(transition.data(),weapon.data()));authority_slot=authority.data();
    player_slot=noaccess;assert(!ready.setbomb(transition.data(),weapon.data()));player_slot=players.data();
    put(players.data(),0x58,noaccess);assert(!ready.setbomb(transition.data(),weapon.data()));put(players.data(),0x58,actor.data());
    DWORD old{};
    assert(VirtualProtect(root,4096,PAGE_READONLY,&old));
    assert(ready.shoot(transition.data(),weapon.data()) && !ready.setbomb(transition.data(),weapon.data()));
    assert(VirtualProtect(root,4096,PAGE_READWRITE,&old));
    assert(VirtualProtect(root+0x4000,4096,PAGE_READONLY,&old));
    assert(!ready.setbomb(transition.data(),weapon.data()));
    put(players.data(),0x58,other.data());assert(ready.setbomb(transition.data(),weapon.data()));
    put(players.data(),0x58,actor.data());assert(VirtualProtect(root+0x4000,4096,PAGE_READWRITE,&old));
    put(actor.data(),0x7810,static_cast<void*>(nullptr));
    assert(ready.setbomb(transition.data(),weapon.data()) && !ready.shoot(transition.data(),weapon.data()));
    assert(store.transact(token,[](auto& s) noexcept {s.counter=8;return true;}));
    assert(ready.shoot(transition.data(),weapon.data()));
    put(actor.data(),0x7810,parameter.data());put(parameter.data(),0,std::uintptr_t{9});
    assert(!ready.setbomb(transition.data(),weapon.data()) && ready.shoot(transition.data(),weapon.data()));
    put(parameter.data(),0,std::uintptr_t{3});assert(both());
    assert(store.retire(token));assert(!ready.shoot(transition.data(),weapon.data()));
    assert(!stormbolt_native_span((std::numeric_limits<std::uintptr_t>::max)()-1,3,4));
    assert(VirtualFree(root,0,MEM_RELEASE) && VirtualFree(noaccess,0,MEM_RELEASE));
    std::puts("PASS native condition readiness: typed ownership/input links, noaccess/readonly pointers, local-only command writes, parameter-optional stage8 and separate synchronous borrow gate.");
}
