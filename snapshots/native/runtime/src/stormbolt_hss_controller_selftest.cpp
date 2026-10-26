#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_hss_gui_controller.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace stormslinger;
namespace {
struct Node {alignas(8) unsigned char bytes[0x198]{};};
struct Fixture {
    std::array<Node,9> nodes{};
    std::array<unsigned char,0x1C8> root{};
    bool hidden{},filter{};
    bool localized_ready=true;
    Node message{},warning_base{};
    int warning_collection{};unsigned text_queries{},text_fits{},warning_plays{};
};
Fixture* fixture{};
template<class T>void put(void* p,std::size_t offset,T value){
    std::memcpy(static_cast<unsigned char*>(p)+offset,&value,sizeof(value));
}
void* root(void*,const char* name){assert(std::strcmp(name,"a_hss_sight")==0);return fixture->root.data();}
void* child(void* collection,const char* name){
    if(collection==&fixture->warning_collection) {
        if(std::strcmp(name,"msg_sign")==0)return &fixture->message;
        if(std::strcmp(name,"null_bace")==0)return &fixture->warning_base;
        assert(false);return nullptr;
    }
    assert(collection==fixture);
    if(std::strcmp(name,"reload_outOfRenge")==0)return &fixture->nodes[8];
    for(unsigned i=0;i<8;++i){char expected[8];std::snprintf(expected,sizeof(expected),"ammo%02u",i+1);
        if(std::strcmp(name,expected)==0)return &fixture->nodes[i];}
    assert(false);return nullptr;
}
std::uint32_t current(void* p){std::uint32_t value{};std::memcpy(&value,p,4);return value;}
void terminal(void* p,std::uint32_t value){put(p,0,value);}
void play(void* p,std::uint32_t value,void* context){
    if(p==&fixture->nodes[8])++fixture->warning_plays;
    assert(context==fixture->root.data());terminal(p,value);
    if(value==0x2711 || value==0x2712){
        auto* function=reinterpret_cast<StormboltHssNativeFunction*>(static_cast<unsigned char*>(p)+0x158);
        (*function)(p,value);
    }
}
bool filter(void*,std::uint32_t channel){assert(channel==0);return fixture->filter;}
void hide_gui(void*,bool hidden){fixture->hidden=hidden;}
void hide_node(void* p,bool hidden){put(p,8,hidden);}
const char* warning_text(void* manager,std::uint32_t id) {
    assert(manager==fixture && id==0x1A);++fixture->text_queries;return fixture->localized_ready?"OUT OF RANGE":"";
}
void fit_warning(void* message,void* base,const char* text) {
    assert(message==&fixture->message && base==&fixture->warning_base);
    assert(std::strcmp(text,"OUT OF RANGE")==0);++fixture->text_fits;
}
}
void hss_controller_selftest(void(*swap)(void*,void*)){
    Fixture state;fixture=&state;put(state.root.data(),0x1C0,fixture);
    for(auto& node:state.nodes)new(node.bytes+0x158) StormboltHssNativeFunction;
    std::vector<unsigned char> actor(0x14000),weapon(0x3000);
    const auto a=reinterpret_cast<std::uintptr_t>(actor.data()),w=reinterpret_cast<std::uintptr_t>(weapon.data());
    put(actor.data(),0,std::uintptr_t(0xAAA));put(actor.data(),0xC,std::uint8_t(0xE));
    put(actor.data(),0x76B0,w);put(actor.data(),0x76A8,std::uint8_t(1));
    put(actor.data(),0x893C,std::uint32_t(8));
    put(weapon.data(),0,std::uintptr_t(0xBBB));put(weapon.data(),0xC,std::uint8_t(0xE));
    put(weapon.data(),0x9B0,a);put(weapon.data(),0x9F8,std::uint32_t(13));
    put(weapon.data(),0x9FC,std::uint32_t(239));put(weapon.data(),0x2388,std::uint32_t(1));
    put(weapon.data(),0x20E0,std::uint8_t(1));put(weapon.data(),0x27C8,std::uint32_t(40));
    StormboltHssGuiBindings api{{root,child},{swap,terminal},{0xAAA,0xBBB},{filter},
        {current,play},hide_gui,hide_node};
    StormboltInstanceStore<4> store;auto token=store.initialize(w);assert(token);
    void* text_manager=fixture;api.text_manager_slot=&text_manager;
    api.text_at=warning_text;api.fit_warning_text=fit_warning;
    // First native initialization can precede the external warning children.
    put(state.nodes[8].bytes,0xF0,static_cast<void*>(nullptr));
    StormboltHssGuiController controller;assert(controller.initialize(&state,api));
    assert(state.text_queries==0 && state.text_fits==0);
    put(state.nodes[8].bytes,0xF0,&state.warning_collection);
    state.localized_ready=false;
    assert(controller.show(a,store));
    assert(state.text_queries==1 && state.text_fits==0);
    put(actor.data(),0x1361C,std::uint8_t(1));
    assert(controller.update(&state,a,store));
    assert(current(&state.nodes[8])==0 && state.text_fits==0);
    state.localized_ready=true;
    assert(controller.update(&state,a,store));
    assert(current(&state.nodes[8])==1 && state.text_queries==3 && state.text_fits==1);
    put(actor.data(),0x1361C,std::uint8_t(0));
    assert(controller.show(a,store));
    for(unsigned stage=0;stage<=8;++stage){
        assert(store.transact(token,[stage](StormboltStageState& value)noexcept{value.counter=stage;return true;}));
        assert(controller.refresh_stage(&state,a,store));
        for(unsigned i=0;i<8;++i)assert(current(&state.nodes[i])==(i<stage?1u:0x2713u));
    }
    put(actor.data(),0x1361C,std::uint8_t(1));assert(controller.update(&state,a,store));
    assert(current(&state.nodes[8])==1);
    // Lifecycle stage callbacks can observe another phase of the transient
    // reload byte. They must not restart or reverse the warning animation.
    const auto warning_plays=state.warning_plays;
    for(unsigned phase=0;phase<6;++phase){
        put(actor.data(),0x1361C,std::uint8_t(phase%2));
        assert(controller.refresh_stage(&state,a,store));
        assert(current(&state.nodes[8])==1 && state.warning_plays==warning_plays);
    }
    put(actor.data(),0x1361C,std::uint8_t(0));assert(controller.update(&state,a,store));
    assert(current(&state.nodes[8])==0x2712);
    for(unsigned gate=0;gate<5;++gate){
        state.hidden=false;
        if(gate==0)put(actor.data(),0x76A8,std::uint8_t(0));
        if(gate==1)put(weapon.data(),0x27C8,std::uint32_t(33));
        if(gate==2)put(weapon.data(),0x2388,std::uint32_t(0));
        if(gate==3)state.filter=true;
        if(gate==4){put(actor.data(),0x6274,std::uint32_t(3));put(actor.data(),0x6278,std::uint32_t(119));}
        // Context sampling cannot request a hide/show reset when another
        // lifecycle phase temporarily fails admission. Stage refresh still
        // owns all five original hide gates.
        assert(controller.update(&state,a,store) && !state.hidden);
        controller.refresh_stage(&state,a,store);assert(state.hidden);
        put(actor.data(),0x76A8,std::uint8_t(1));put(weapon.data(),0x27C8,std::uint32_t(40));
        put(weapon.data(),0x2388,std::uint32_t(1));state.filter=false;
        put(actor.data(),0x6274,std::uint32_t(0));put(actor.data(),0x6278,std::uint32_t(0));
    }
    // A deactivated native weapon can remain tracked until its destructor.
    put(weapon.data(),0xC,std::uint8_t(0x10));state.hidden=false;
    assert(controller.update(&state,a,store) && !state.hidden);
    assert(!controller.refresh_stage(&state,a,store) && state.hidden);
    StormboltStageState retained{};assert(store.snapshot(token,retained) && retained.counter==8);
    put(weapon.data(),0xC,std::uint8_t(0xE));
    assert(store.retire(token));token=store.initialize(w);assert(token);
    assert(controller.refresh_stage(&state,a,store));
    for(unsigned i=0;i<8;++i)assert(current(&state.nodes[i])==0x2713);
    assert(state.text_queries==3 && state.text_fits==1); // Completed text is not refitted each frame.
    controller.retire();assert(!controller.update(&state,a,store));
    for(auto& node:state.nodes)reinterpret_cast<StormboltHssNativeFunction*>(node.bytes+0x158)->~StormboltHssNativeFunction();
    fixture=nullptr;
    std::puts("PASS:HSS controller input/store/9 stages/reload/5 hide gates/generation reset through native callback swap; GUI APIs are fixtures");
}
