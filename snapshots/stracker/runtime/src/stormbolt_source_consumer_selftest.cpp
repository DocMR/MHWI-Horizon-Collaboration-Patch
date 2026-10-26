#ifdef NDEBUG
#undef NDEBUG
#endif
#include <Windows.h>
#include "stormbolt_source_graph_session.h"
#include "stormbolt_shoot_gatling_predicate.h"
#include "stormbolt_source_transition_predicate.h"
#include "stormbolt_source_transition_condition_hook.h"
#include "generated_source_consumer_fixture.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
struct Condition {
    void** table=nullptr;
    std::uint64_t flags=0;
    StormboltShootGatlingInput input{};
    int result=-1;
    bool core=false;
    bool source=false;
};
struct SourceInput {
    std::int32_t remaining=1,command=0;
    std::uint32_t count=1,calls=0;
    bool mode=false,authority=true,direct=false;
    float charge=1;
} source_input;
const StormboltSourceTransitionPredicateApi source_api{
    +[](void*){return source_input.remaining;},
    +[](void*){return source_input.direct;},
    +[]{return false;},+[]{return source_input.authority;},
    +[](void*){return source_input.count;},+[](void*){return source_input.mode;},
    +[](void*,float& ratio){ratio=source_input.charge;return true;},
    +[](void*){++source_input.calls;return source_input.command;},
    +[](void*){return true;}};
int predicate(void* p) {
    auto& c=*static_cast<Condition*>(p);
    if(c.source) return stormbolt_evaluate_source_transition(p,&source_input,source_api);
    return c.core?stormbolt_evaluate_shoot_gatling(c.input):c.result;
}
unsigned original_mode_calls=0;
bool hook_ready=true;
int original_mode(void*) {++original_mode_calls;return -1;}
template<class T,class Object> void put(Object& object,std::size_t offset,T value) {
    std::memcpy(object.data()+offset,&value,sizeof(value));
}
int main() {
    constexpr std::size_t image_size=0x2800000;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(image);
    for(const auto& span:source_consumer_spans) std::memcpy(image+span.rva,span.data,span.size);
    image[0x273D36C]=0xCC;
    for(const auto& span:source_consumer_spans) {
        DWORD old{};assert(VirtualProtect(image+span.rva,span.size,PAGE_EXECUTE_READ,&old));
    }
    DWORD old{};assert(VirtualProtect(image+0x273D36C,1,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),image,image_size));
    using Select=StormboltGraphLink*(*)(void*,void*,unsigned*,StormboltGraphLink*,unsigned,bool);
    auto select=reinterpret_cast<Select>(image+0x26B830);
    std::array<void*,9> table{};table[6]=reinterpret_cast<void*>(&predicate);
    std::array<Condition,799> objects{};
    std::array<void*,798> originals{};
    for(unsigned n=0;n<798;++n) {objects[n].table=table.data();originals[n]=&objects[n];}
    objects[798].table=table.data();objects[798].core=true;
    alignas(8) std::array<unsigned char,0x2B0> machine{};
    auto** registry_slot=reinterpret_cast<void***>(machine.data()+0xD0);
    auto* registry_count=reinterpret_cast<std::uint32_t*>(machine.data()+0xD8);
    *registry_slot=originals.data();*registry_count=798;
    std::array<StormboltGraphState,120> states{};
    std::vector<std::vector<StormboltGraphLink>> links;
    links.reserve(std::size(source_graph_edits));
    for(const auto& edit:source_graph_edits) {
        links.emplace_back(edit.expected,edit.expected+edit.count);
        states[edit.state].lists[edit.secondary]={links.back().data(),static_cast<std::uint32_t>(edit.count),0};
    }
    StormboltSourceGraphSession session;
    assert(session.install(states.data(),120,registry_slot,registry_count,&objects[798]));
    unsigned cases=0;
    for(const auto& edit:source_graph_edits) {
        auto& list=states[edit.state].lists[edit.secondary];
        for(unsigned scenario=0;scenario<4;++scenario) {
            auto& in=objects[798].input;in={};in.ammo=scenario?40:0;
            in.stage=scenario==3?8:0;in.held=true;in.resource=1;
            in.stage_mode=scenario==2?1:0;
            unsigned selected_index=0xFFFFFFFF;
            auto* selected=select(machine.data(),nullptr,&selected_index,list.links,list.count,true);
            const int expected=edit.secondary?(scenario==1?20:scenario==2?21:-1):(scenario==3?119:-1);
            assert((selected!=nullptr)==(expected>=0));
            if(selected) assert(selected->target_group==3 && selected->target_key==expected);
            ++cases;
        }
        if(edit.secondary) for(int result:{2,3}) {
            objects[776].result=result;
            unsigned selected_index=0xFFFFFFFF;
            auto* selected=select(machine.data(),nullptr,&selected_index,list.links,list.count,true);
            assert(selected && selected->target_group==3 && selected->target_key==(result==2?18:19));
            ++cases;
        }
        objects[776].result=-1;
        if(edit.secondary) {
            // Compose the actual mode predicate with the retained native graph
            // consumer, rather than directly injecting its return value2/3.
            objects[798].input={};objects[776].source=true;
            for(unsigned scenario=0;scenario<6;++scenario) {
                source_input={};
                if(scenario==1 || scenario==3) source_input.mode=true;
                if(scenario==2) source_input.command=-1;
                if(scenario==3) {source_input.direct=true;source_input.charge=0;}
                if(scenario==4) source_input.count=0;
                if(scenario==5) {source_input.authority=false;source_input.remaining=0;}
                unsigned selected_index=0xFFFFFFFF;
                auto* selected=select(machine.data(),nullptr,&selected_index,list.links,list.count,true);
                const int expected=scenario==0?18:(scenario==1 || scenario==3)?19:-1;
                if(expected>=0) assert(selected && selected->target_group==3 && selected->target_key==expected);
                else if(scenario==4) {
                    unsigned original_index=0xFFFFFFFF;
                    auto* original=select(machine.data(),nullptr,&original_index,
                        const_cast<StormboltGraphLink*>(edit.expected),static_cast<unsigned>(edit.count),true);
                    assert(bool(original)==bool(selected));
                    if(original) assert(!std::memcmp(original,selected,sizeof(*original)));
                } else assert(!selected);
                const auto expected_calls=(scenario==3 || scenario==5)?0u:1u;
                // Native lists can reference a predicate more than once; only
                // automatic exit and early rejection must avoid commands.
                if(expected_calls==0) assert(source_input.calls==0);
                else assert(source_input.calls>=1);
                ++cases;
            }
            objects[776].source=false;
        }
    }
    // Native graph -> installed production condition callback -> actual predicate.
    // Native API reads/command and object memory remain explicit fixtures.
    StormboltSourceTransitionLifecycle<2>::Store condition_store;
    StormboltSourceTransitionConditionHook<2> condition_hook(condition_store);
    alignas(8) std::array<void*,7> mode_table{};
    std::array<void*,1> actor_table{},weapon_table{};
    alignas(8) std::array<unsigned char,0x38> mode_object{};
    alignas(8) std::array<unsigned char,0x76B8> actor{};
    alignas(8) std::array<unsigned char,0xA00> weapon{};
    mode_table[6]=reinterpret_cast<void*>(&original_mode);
    put(mode_object,0,mode_table.data());put(mode_object,0x10,actor.data());
    put(actor,0,actor_table.data());put(actor,0xC,std::uint8_t{2});put(actor,0x76B0,weapon.data());
    put(weapon,0,weapon_table.data());put(weapon,0xC,std::uint8_t{2});put(weapon,0x9B0,actor.data());
    put(weapon,0x9F8,std::uint32_t{13});put(weapon,0x9FC,std::uint32_t{239});
    StormboltSourceTransitionConditionBindings bindings{
        reinterpret_cast<std::uintptr_t>(mode_table.data()),
        reinterpret_cast<std::uintptr_t>(actor_table.data()),
        reinterpret_cast<std::uintptr_t>(weapon_table.data()),&original_mode,
        +[](void*,void*){return hook_ready;}};
    assert(condition_hook.install(bindings,source_api));
    auto** active_registry=*registry_slot;
    auto* saved_mode_object=active_registry[776];
    active_registry[776]=mode_object.data();
    for(const auto& edit:source_graph_edits) if(edit.secondary) {
        const auto& list=states[edit.state].lists[edit.secondary];
        for(unsigned scenario=0;scenario<8;++scenario) {
            source_input={};hook_ready=true;original_mode_calls=0;
            put(weapon,0x9FC,std::uint32_t{239});
            const auto token=condition_store.initialize(reinterpret_cast<std::uintptr_t>(mode_object.data()));
            if(scenario==1 || scenario==3) source_input.mode=true;
            if(scenario==2) source_input.command=-1;
            if(scenario==3) {source_input.direct=true;source_input.charge=0;}
            if(scenario==4) source_input.count=0;
            if(scenario==5) condition_store.retire(token);
            if(scenario==6) hook_ready=false;
            if(scenario==7) put(weapon,0x9FC,std::uint32_t{237});
            objects[798].input={};
            unsigned selected_index=0xFFFFFFFF;
            auto* selected=select(machine.data(),nullptr,&selected_index,list.links,list.count,true);
            const int expected=scenario==0?18:(scenario==1 || scenario==3)?19:-1;
            assert(bool(selected)==(expected>=0));
            if(selected) assert(selected->target_group==3 && selected->target_key==expected);
            if(scenario>=4) assert(original_mode_calls>=1 && source_input.calls==0);
            else {
                assert(original_mode_calls==0);
                if(scenario==3) assert(source_input.calls==0);
                else assert(source_input.calls>=1);
            }
            ++cases;
        }
    }
    active_registry[776]=saved_mode_object;
    assert(condition_hook.restore() && mode_table[6]==reinterpret_cast<void*>(&original_mode));
    assert(session.restore() && *registry_slot==originals.data() && *registry_count==798);
    assert(VirtualFree(image,0,MEM_RELEASE));
    std::printf("PASS%u native graph cases including installed production mode hook, identity/readiness/no-part original routes, automatic exit and exact restore.\n",cases);
}
