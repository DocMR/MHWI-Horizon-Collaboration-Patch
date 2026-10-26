#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_motion_event_hook.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
const void* seen_root{};
void* seen_player{};
void* seen_parameters{};
float seen_frame{};
bool seen_reset{},ready=true;
unsigned calls=0;
__declspec(noinline) void native_bind(void* player,const void* root,void* parameters,float frame,bool reset) {
    seen_player=player;seen_root=root;seen_parameters=parameters;
    seen_frame=frame;seen_reset=reset;++calls;
}
template<class T,class Object> void put(Object& object,std::size_t offset,T value) {
    std::memcpy(object.data()+offset,&value,sizeof(value));
}
}
int main() {
    StormboltInstanceStore<2> store;
    StormboltMotionEventHook<2> hook(store);
    HookTransaction transaction;
    alignas(8) std::array<unsigned char,0x76B8> actor{};
    alignas(8) std::array<unsigned char,0x2390> weapon{};
    alignas(8) std::array<unsigned char,0xE200> component{};
    std::array<void*,1> actor_table{},weapon_table{};
    std::array<unsigned char,0xB0> resource{},other_resource{};
    std::array<unsigned char,0x10+256*8> lmt{};
    std::array<std::array<unsigned char,96>,256> motions{};
    std::array<std::uint64_t,256> native_roots{};
    std::array<const void*,256> native_root_pointers{};
    for(unsigned i=0;i<256;++i) native_root_pointers[i]=&native_roots[i];
    put(resource,0xA8,lmt.data());
    put(lmt,0,std::uint32_t{0x00544D4C});put(lmt,6,std::uint16_t{256});
    for(unsigned i=0;i<256;++i) {
        put(lmt,0x10+i*8,motions[i].data());
        put(motions[i],0x58,native_root_pointers[i]);
    }
    put(actor,0,actor_table.data());put(actor,0xC,std::uint8_t{2});
    put(actor,0x468,component.data());put(actor,0x76B0,weapon.data());
    put(weapon,0,weapon_table.data());put(weapon,0xC,std::uint8_t{2});
    put(weapon,0x9B0,actor.data());put(weapon,0x9F8,std::uint32_t{13});
    put(weapon,0x9FC,std::uint32_t{239});put(weapon,0x2388,std::uint32_t{1});
    put(component,0xE120+12*8,&resource);
    auto token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));assert(token);
    assert(transaction.begin()==MH_OK);
    assert(hook.prepare(transaction,{&native_bind,reinterpret_cast<std::uintptr_t>(actor_table.data()),
        reinterpret_cast<std::uintptr_t>(weapon_table.data()),+[](void*){return ready;},nullptr})==MH_OK);
    assert(transaction.commit()==MH_OK);
    StormboltTimelineBind volatile invoke=&native_bind;
    const std::uint64_t original_root=0xABCDEF;
    unsigned cases=0;
    for(unsigned channel=0;channel<6;++channel) for(unsigned variant=0;variant<4;++variant) {
        const auto offset=0x80+channel*0x2570+0x30+variant*0x1B0+0x148;
        auto* player=component.data()+offset;
        auto* parameters=actor.data()+0x6B8+channel*0x40;
        put(component,offset-0x10,actor_table.data());
        put(component,offset-8,&resource);
        put(component,offset+0x20,parameters);
        for(const auto& definition:stormbolt_motion_event_roots) {
            // Native frame is deliberately not a motion ID; cached old ID is unrelated.
            put(component,offset+0x40,12.5f);
            put(component,offset+0x54,std::uint32_t{17});
            const float frame=static_cast<float>(definition.slot)+0.5f;
            const bool reset=bool(variant&1);
            const auto before=calls;
            invoke(player,native_root_pointers[definition.slot],parameters,frame,reset);
            assert(calls==before+1 && seen_player==player && seen_parameters==parameters);
            assert(seen_frame==frame && seen_reset==reset);
            assert(seen_root!=native_root_pointers[definition.slot]);
            assert(bool(seen_root)==(definition.offset!=0xFFFFFFFFu));
            ++cases;
        }
    }
    const auto offset=0x80+0x2570+0x30+0x148;
    auto* player=component.data()+offset;
    auto* parameters=actor.data()+0x6F8;
    put(component,offset+0x40,std::uint32_t{244});
    for(unsigned scenario=0;scenario<10;++scenario) {
        put(actor,0xC,std::uint8_t{2});put(weapon,0xC,std::uint8_t{2});
        ready=true;put(weapon,0x2388,std::uint32_t{1});put(weapon,0x9FC,std::uint32_t{239});
        put(component,offset-0x10,actor_table.data());
        put(component,offset-8,&resource);
        put(component,offset+0x20,parameters);put(weapon,0x9B0,actor.data());
        put(component,offset+0x40,std::uint32_t{244});
        token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));
        if(scenario==0) put(weapon,0x2388,std::uint32_t{0});
        if(scenario==1) put(weapon,0x9FC,std::uint32_t{237});
        if(scenario==2) put(component,offset-8,&other_resource);
        if(scenario==3) put(weapon,0x9B0,component.data());
        if(scenario==4) ready=false;
        if(scenario==5) store.retire(token);
        if(scenario==8) put(weapon,0xC,std::uint8_t{0x10});
        if(scenario==9) put(actor,0xC,std::uint8_t{0x10});
        const void* incoming=scenario==6 ? &original_root : native_root_pointers[244];
        const auto before=calls;
        invoke(player,incoming,scenario==7 ? static_cast<void*>(parameters+8) : parameters,0.5f,true);
        assert(calls==before+1 && seen_root==incoming && seen_frame==0.5f && seen_reset);
        ++cases;
    }
    put(actor,0xC,std::uint8_t{2});put(weapon,0xC,std::uint8_t{2});
    ready=true;token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));assert(token);
    hook.withdraw_while_quiescent();
    invoke(player,&original_root,parameters,1.0f,false);
    assert(seen_root==&original_root);
    assert(transaction.stop()==MH_OK);hook.release_after_stop();
    invoke(player,&original_root,parameters,2.0f,true);
    assert(seen_root==&original_root && seen_frame==2.0f && seen_reset);
    std::printf("PASS:%u installed binding-hook cases, all6 layers/4 variants, explicit empty roots, complete5-argument ABI, scoped fallback and restore.\n",cases);
}
