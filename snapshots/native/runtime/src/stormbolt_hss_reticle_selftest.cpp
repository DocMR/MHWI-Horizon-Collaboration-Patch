#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_hss_reticle.h"
#include "stormbolt_hss_native_callbacks.h"
#include "stormbolt_hss_native_input.h"
#include "stormbolt_hss_gui_controller.h"
#include "stormbolt_hss_native_type.h"
#include "generated_stormbolt_hss_cases.h"
#include "generated_stormbolt_hss_function_fixture.h"
#include <Windows.h>
#include <memory>
#include <cassert>
#include <cstdio>
using namespace stormslinger;
void hss_catalog_selftest();
void hss_controller_selftest(void(*)(void*,void*));
template class stormslinger::StormboltHssNativeType<64>;
namespace {
struct Mark {std::uint32_t current,request=0;};
std::uint32_t animation(void* p){return static_cast<Mark*>(p)->current;}
void play(void* p,std::uint32_t request,void* context){assert(context==reinterpret_cast<void*>(0x1234));auto& mark=*static_cast<Mark*>(p);assert(!mark.request);mark.request=request;}
unsigned terminal_calls{};void* terminal_node{};std::uint32_t terminal_request{};
std::size_t gui_object_size{};unsigned gui_destroys{},gui_removes{};bool deferred_remove{};
void type_register(void* type,const char* name,void*,std::size_t size,unsigned,unsigned,unsigned){
    gui_object_size=size;std::memcpy(static_cast<unsigned char*>(type)+8,&name,8);
}
void* gui_base_construct(void* p){std::memset(p,0,0x28F0);return p;}
void gui_base_destroy(void*){++gui_destroys;}
void gui_priority(void*,unsigned value){assert(value==5);}
void gui_request(void*,void*){}
void* gui_actor(){return nullptr;}
void gui_hide(void*,bool hidden){assert(hidden);}
void destroy_fixture_gui(void* p){void** table{};std::memcpy(&table,p,8);
    reinterpret_cast<void*(*)(void*,unsigned)>(table[0])(p,0);}
void gui_remove(void* p){++gui_removes;if(!deferred_remove)destroy_fixture_gui(p);}
void native_type_lifecycle(){
    static StormboltHssNativeType<64> type;static StormboltInstanceStore<64> store;
    std::array<void*,0x2E0/8> base{};std::array<void*,5> factories{};
    StormboltHssTypeBindings api{};api.register_type=type_register;
    api.parent_type=reinterpret_cast<void*>(1);api.base_vtable=base.data();api.dti_vtable=factories.data();
    api.construct_base=gui_base_construct;api.destroy_base=gui_base_destroy;
    api.set_priority=gui_priority;api.request_show=gui_request;api.remove_gui=gui_remove;
    api.local_actor=gui_actor;api.gui.hide_gui=gui_hide;
    assert(type.register_type(api,store));assert(gui_object_size>0x28F0 && gui_object_size%16==0);
    void** table{};std::memcpy(&table,type.type(),8);
    const auto place_array=reinterpret_cast<void*(*)(void*,void*,unsigned)>(table[3]);
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,gui_object_size*2,
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(memory);
    assert(place_array(type.type(),memory,2)==memory);
    void** methods{};std::memcpy(&methods,memory,sizeof(methods));
    assert(methods[0x268/8]!=methods[0x280/8]);
    assert(methods[0x280/8]==methods[0x288/8] && methods[0x288/8]==methods[0x2A0/8]);
    type.retire_instances();assert(gui_removes==2 && gui_destroys==2);
    type.retire_instances();assert(gui_removes==2);
    deferred_remove=true;assert(place_array(type.type(),memory,2)==memory);
    type.retire_instances();type.retire_instances();assert(gui_removes==4 && gui_destroys==2);
    destroy_fixture_gui(memory);destroy_fixture_gui(memory+gui_object_size);
    type.retire_instances();assert(gui_destroys==4 && gui_removes==4);
    assert(VirtualFree(memory,0,MEM_RELEASE));
    std::puts("PASS:HSS concrete type placement/array stride and synchronous/deferred retirement; base GUI APIs are fixtures");
}
void terminal(void* node,std::uint32_t request){++terminal_calls;terminal_node=node;terminal_request=request;}
void native_callback_abi(){
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(code);
    std::memcpy(code,hss_native_move,sizeof(hss_native_move));
    constexpr auto swap_offset=0x24B980-0x247B70;
    std::memcpy(code+swap_offset,hss_native_swap,sizeof(hss_native_swap));
    DWORD previous{};assert(VirtualProtect(code,0x4000,PAGE_EXECUTE_READ,&previous));
    assert(FlushInstructionCache(GetCurrentProcess(),code,0x4000));
    const auto swap=reinterpret_cast<void(*)(void*,void*)>(code+swap_offset);
    struct Node {alignas(8) unsigned char bytes[0x198]{};};
    std::array<Node,9> nodes{};std::array<std::weak_ptr<int>,9> old{};
    StormboltHssBoundNodes bound{};
    for(unsigned i=0;i<9;++i){
        auto keep=std::make_shared<int>(1);old[i]=keep;
        // Alternate inline and heap callables to exercise both native branches.
        auto* storage=new(nodes[i].bytes+0x158) StormboltHssNativeFunction;
        if(i%2) *storage=[keep,padding=std::array<unsigned char,64>{}](void* const&,const std::uint32_t&){assert(keep && padding[0]==0);};
        else *storage=[keep](void* const&,const std::uint32_t&){assert(keep);};
        if(i<8)bound.stages.marks[i]=&nodes[i];else bound.reload_out_of_range=&nodes[i];
    }
    assert(install_stormbolt_hss_callbacks(bound,{swap,terminal}));
    for(unsigned i=0;i<9;++i){
        assert(old[i].expired());
        auto* storage=nodes[i].bytes+0x158;void* active{};
        std::memcpy(&active,storage+0x38,sizeof(active));assert(active);
        void** table{};std::memcpy(&table,active,sizeof(table));
        const auto invoke=reinterpret_cast<void(*)(void*,void* const&,const std::uint32_t&)>(table[2]);
        void* node=&nodes[i];
        for(const auto event:{0x2711u,0x2712u,0x2713u}){
            const auto before=terminal_calls;invoke(active,node,event);
            const bool expected=event==0x2711u || (i<8 && event==0x2712u);
            assert(terminal_calls==before+unsigned(expected));
            if(expected){assert(terminal_node==node);assert(terminal_request==(event==0x2711u?1u:0x2713u));}
        }
        StormboltHssNativeFunction empty;swap(&empty,storage);
        reinterpret_cast<StormboltHssNativeFunction*>(storage)->~StormboltHssNativeFunction();
    }
    hss_controller_selftest(swap);
    assert(VirtualFree(code,0,MEM_RELEASE));
}
}
void native_node_terminal_layout() {
    void* code=VirtualAlloc(nullptr,sizeof(hss_native_node_terminal),
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(code);
    std::memcpy(code,hss_native_node_terminal,sizeof(hss_native_node_terminal));
    DWORD old{};assert(VirtualProtect(code,sizeof(hss_native_node_terminal),PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),code,sizeof(hss_native_node_terminal)));
    std::array<unsigned char,0x200> node{};
    const std::uint64_t poison=0xBF2787F43E024234ull;
    std::memcpy(node.data()+0x1C0,&poison,8);
    auto play=reinterpret_cast<void(*)(void*,std::uint32_t)>(code);
    stormbolt_hss_animation_event(node.data(),0x2711,true,play);
    std::uint32_t animation{};std::memcpy(&animation,node.data()+0xE4,4);assert(animation==1);
    stormbolt_hss_animation_event(node.data(),0x2712,true,play);
    std::memcpy(&animation,node.data()+0xE4,4);assert(animation==0x2713);
    std::uint64_t preserved{};std::memcpy(&preserved,node.data()+0x1C0,8);assert(preserved==poison);
    assert(VirtualFree(code,0,MEM_RELEASE));
    std::puts("PASS: actual PC2342AF0 terminal callback writes nodeE4 with null resource and poisoned1C0; no GUI layout read");
}
int main(){
    native_node_terminal_layout();
    native_type_lifecycle();
    hss_catalog_selftest();
    native_callback_abi();
    for(const auto& test:hss_animation_cases){
        std::array<Mark,8> marks{};StormboltHssReticleNodes nodes{};nodes.context=reinterpret_cast<void*>(0x1234);
        for(unsigned i=0;i<8;++i){marks[i].current=test.representation==1?0xFFFFFFFFu:test.current;nodes.marks[i]=&marks[i];}
        assert(refresh_stormbolt_hss_reticle(nodes,test.stage,test.immediate,{animation,play}));
        for(unsigned i=0;i<8;++i)assert(marks[i].request==test.requests[i]);
    }
    StormboltHssReticleNodes missing{};assert(!refresh_stormbolt_hss_reticle(missing,8,false,{animation,play}));
    std::puts("PASS:native PC callback swap/move/invoke/destruction for 9 nodes and 27 events;330 PS animation cases; GUI creation/rendering not tested");
}
