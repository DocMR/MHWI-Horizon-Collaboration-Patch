#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_source_transition_predicate.h"
#include "stormbolt_source_transition_condition_hook.h"
#include "stormbolt_source_transition_native_fields.h"
#include "generated_source_transition_predicate_cases.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <cmath>
using namespace stormslinger;
namespace {
struct World {const SourcePredicateCase* test=nullptr;unsigned bits=0;std::uint64_t trace=0;} world;
using ConditionStore=StormboltSourceTransitionLifecycle<2>::Store;
ConditionStore* condition_store=nullptr;
void* hook_transition=nullptr;
void* hook_weapon=nullptr;
bool reset_on_command=false,reset_on_ready=false,ready_value=true;
unsigned original_calls=0,ready_calls=0;
std::int32_t original_condition(void*) {++original_calls;return -7;}
std::int32_t foreign_condition(void*) {return 55;}
bool ready_condition(void* transition,void* weapon) {
    assert(transition==hook_transition && weapon==hook_weapon);++ready_calls;
    if(reset_on_ready) condition_store->initialize(reinterpret_cast<std::uintptr_t>(transition));
    return ready_value;
}
void event(unsigned id) {world.trace=(world.trace<<4)|id;}
bool bit(unsigned n) {return (world.bits&(1u<<n))!=0;}
void check(void* object) {assert(object==&world || object==hook_transition || object==hook_weapon);}
std::int32_t remaining(void* p) {check(p);event(1);return bit(0)?1:0;}
bool direct(void* p) {check(p);event(2);return bit(1);}
bool platform() {event(3);return bit(2);}
bool authority() {event(4);return bit(3);}
std::uint8_t authority_context(void* context,std::uint32_t index) {
    assert(context==&world && index==0);return authority()?1:0;
}
std::uint32_t source(void* p) {check(p);event(5);return bit(4)?1u:0u;}
bool mode(void* p) {check(p);event(6);return bit(5);}
bool ratio(void* p,float& value) {
    check(p);event(7);
    const float choices[]={0,-1,0,1,std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()};
    value=choices[world.test->ratio];return world.test->ratio!=0;
}
std::int32_t command(void* p) {
    check(p);event(8);world.bits^=world.test->mutate<<2;
    if(reset_on_command) condition_store->initialize(reinterpret_cast<std::uintptr_t>(p));
    return world.test->command;
}
bool mine(void* p) {check(p);event(9);return world.test->mine;}
std::uint64_t source_gate_trace(std::uint64_t body) {
    auto prefix=std::uint64_t{5};
    for(auto value=body;value;value>>=4) prefix<<=4;
    return prefix|body;
}
template<class T,class Object> void put(Object& object,std::size_t offset,T value) {
    std::memcpy(object.data()+offset,&value,sizeof(value));
}
}
int main() {
    const StormboltSourceTransitionPredicateApi api{
        remaining,direct,platform,authority,source,mode,ratio,command,mine};
    assert(api.complete());
    for(const auto& test:source_predicate_cases) {
        world={&test,test.bits,0};
        StormboltSourceTransitionObservation observation{};
        const auto result=stormbolt_evaluate_source_transition(&world,&world,api,&observation);
        bool command_seen=false;
        for(auto trace=world.trace;trace;trace>>=4) if((trace&15)==8) command_seen=true;
        assert(observation.direct==bool(test.bits&2) && observation.command_called==command_seen);
        assert(observation.command_result==(command_seen?test.command:-1));
        if(result!=test.expected || world.trace!=test.trace) {
            std::printf("Mismatch bits=%u ratio=%u cmd=%d mine=%u mutate=%u: result%d/%d trace%llX/%llX\n",
                test.bits,test.ratio,test.command,unsigned(test.mine),test.mutate,result,test.expected,
                static_cast<unsigned long long>(world.trace),static_cast<unsigned long long>(test.trace));
            return 1;
        }
    }
    auto contextual=api;
    void* authority_object=&world;
    contextual.platform_blocked=nullptr;contextual.authority=nullptr;
    contextual.native_platform_blocked=&platform;
    contextual.native_authority_slot=&authority_object;
    contextual.native_authority_query=&authority_context;
    assert(contextual.complete());
    for(const auto& test:source_predicate_cases) {
        world={&test,test.bits,0};
        assert(stormbolt_evaluate_source_transition(&world,&world,contextual)==test.expected);
        assert(world.trace==test.trace);
    }
    ConditionStore store;condition_store=&store;
    alignas(8) std::array<void*,7> transition_table{};
    std::array<void*,1> actor_table{},weapon_table{};
    std::array<unsigned char,0x38> transition{};
    std::array<unsigned char,0x76B8> actor{};
    std::array<unsigned char,0xA00> weapon{};
    hook_transition=transition.data();hook_weapon=weapon.data();
    transition_table[6]=reinterpret_cast<void*>(&original_condition);
    put(transition,0,transition_table.data());put(transition,0x10,actor.data());
    put(actor,0,actor_table.data());put(actor,0xC,std::uint8_t{2});put(actor,0x76B0,weapon.data());
    put(weapon,0,weapon_table.data());put(weapon,0xC,std::uint8_t{2});put(weapon,0x9B0,actor.data());
    put(weapon,0x9F8,std::uint32_t{13});put(weapon,0x9FC,std::uint32_t{239});
    const StormboltSourceTransitionConditionBindings bindings{
        reinterpret_cast<std::uintptr_t>(transition_table.data()),reinterpret_cast<std::uintptr_t>(actor_table.data()),
        reinterpret_cast<std::uintptr_t>(weapon_table.data()),&original_condition,&ready_condition};
    StormboltSourceTransitionConditionHook<2> hook(store);
    assert(hook.install(bindings,api));
    auto invoke=[&](void* object) {return reinterpret_cast<std::int32_t (*)(void*)>(transition_table[6])(object);};
    // An unknown old object is forwarded before ANY native memory read.
    auto* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(inaccessible);
    assert(invoke(inaccessible)==-7 && original_calls==1 && ready_calls==0);
    assert(VirtualFree(inaccessible,0,MEM_RELEASE));
    assert(invoke(transition.data())==-7 && original_calls==2 && ready_calls==0);
    store.initialize(reinterpret_cast<std::uintptr_t>(transition.data()));
    original_calls=0;
    unsigned no_source_cases=0;
    for(const auto& test:source_predicate_cases) {
        world={&test,test.bits,0};
        const auto before_original=original_calls;
        const auto saved_transition=transition;
        const auto saved_weapon=weapon;
        if(!(test.bits&(1u<<4))) {
            ++no_source_cases;
            assert(invoke(transition.data())==-7 && world.trace==5);
            assert(original_calls==before_original+1);
            assert(transition==saved_transition && weapon==saved_weapon);
        } else {
            assert(invoke(transition.data())==test.expected && world.trace==source_gate_trace(test.trace));
            assert(original_calls==before_original);
        }
    }
    assert(no_source_cases && original_calls==no_source_cases && ready_calls==3648);
    original_calls=0;
    put(weapon,0x9FC,std::uint32_t{237});world.trace=0;
    assert(invoke(transition.data())==-7 && original_calls==1 && world.trace==0);
    put(weapon,0x9FC,std::uint32_t{238});
    ready_value=false;
    assert(invoke(transition.data())==-7 && original_calls==2 && world.trace==0);
    ready_value=true;reset_on_ready=true;
    assert(invoke(transition.data())==-7 && original_calls==3 && world.trace==0);
    reset_on_ready=false;
    const SourcePredicateCase command_case{17,0,0,false,0,0,0};
    world={&command_case,17,0};reset_on_command=true;
    assert(invoke(transition.data())==-1 && original_calls==3 && world.trace!=0);
    reset_on_command=false;
    const auto installed=transition_table[6];transition_table[6]=reinterpret_cast<void*>(&foreign_condition);
    assert(!hook.restore() && hook.active());
    transition_table[6]=installed;assert(hook.restore());
    assert(invoke(transition.data())==-7 && original_calls==4);
    auto contextual_bindings=bindings;
    contextual_bindings.ready=nullptr;contextual_bindings.ready_context=&world;
    contextual_bindings.ready_with_context=+[](void* t,void* w,void* context) {
        assert(context==&world);return ready_condition(t,w);
    };
    ready_calls=0;
    assert(hook.install(contextual_bindings,api));
    for(const auto& test:source_predicate_cases) {
        world={&test,test.bits,0};
        const auto before_original=original_calls;
        if(!(test.bits&(1u<<4))) {
            assert(invoke(transition.data())==-7 && world.trace==5);
            assert(original_calls==before_original+1);
        } else {
            assert(invoke(transition.data())==test.expected && world.trace==source_gate_trace(test.trace));
            assert(original_calls==before_original);
        }
    }
    assert(ready_calls==3648 && hook.restore());
    // Actual PC field adapters: keep the remaining/count/mode fields distinct.
    std::array<unsigned char,0x2860> native_weapon{};
    std::array<unsigned char,0xE488> native_actor{};
    std::array<unsigned char,0x9C4> native_parameter{};
    const auto fields=stormbolt_source_native_field_api();
    assert(!fields.complete() && fields.direct_toggle && !fields.command && !fields.authority);
    put(native_weapon,0x2848,std::int32_t{-3});put(native_weapon,0x2388,std::uint32_t{4});
    put(native_weapon,0x2350,std::uint8_t{2});
    assert(fields.remaining(native_weapon.data())==-3 && fields.source_count(native_weapon.data())==4);
    assert(fields.special_mode(native_weapon.data()));
    float output=123;
    assert(!fields.charge_ratio(native_weapon.data(),output) && output==123);
    put(native_weapon,0x9B0,native_actor.data());
    assert(!fields.charge_ratio(native_weapon.data(),output) && output==123);
    put(native_actor,0xC,std::uint8_t{2});
    assert(!fields.charge_ratio(native_weapon.data(),output) && output==123);
    put(native_actor,0x7810,native_parameter.data());
    const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN();
    struct RatioCase {float charge,limit,expected;};
    const RatioCase ratios[]={{2,2,1},{-2,2,-1},{0,2,0},{2,0,inf},{-2,0,-inf},
        {0,0,nan},{nan,2,nan},{inf,2,inf},{-inf,2,-inf},{2,inf,0},{inf,inf,nan}};
    for(const auto& test:ratios) {
        put(native_weapon,0x2358,test.charge);put(native_parameter,0x9C0,test.limit);
        const auto saved_weapon=native_weapon;const auto saved_actor=native_actor;
        const auto saved_parameter=native_parameter;
        assert(fields.charge_ratio(native_weapon.data(),output));
        assert(std::isnan(test.expected) ? std::isnan(output) : output==test.expected);
        assert(native_weapon==saved_weapon && native_actor==saved_actor && native_parameter==saved_parameter);
    }
    std::array<unsigned char,0x38> native_transition{},native_input{};
    std::array<unsigned char,0xA0> physical_input{};
    put(native_transition,0x20,native_input.data());put(native_input,0x30,native_actor.data());
    put(native_actor,0xE480,physical_input.data());
    for(const auto& test:source_direct_input_cases) {
        put(native_transition,8,std::uint8_t(test.flag));
        put(native_actor,0xE44C,std::uint8_t((test.prefix>>4)&1));
        put(physical_input,8,(test.prefix&1)?std::int32_t{-1}:std::int32_t{0});
        put(physical_input,0x21,std::uint8_t((test.prefix&2)?8:0));
        put(physical_input,0x68,(test.prefix&4)?std::int32_t{-1}:std::int32_t{0});
        put(physical_input,0x81,std::uint8_t((test.prefix&8)?8:0));
        const auto saved_transition=native_transition,saved_input=native_input;
        const auto saved_physical=physical_input;
        assert(fields.direct_toggle(native_transition.data())==test.expected);
        assert(native_transition==saved_transition && native_input==saved_input && physical_input==saved_physical);
    }
    std::puts("PASS:3648 PS instruction-oracle cases through core AND owned condition slot; old/unreadable objects and non-target fallback, readiness generation reset, no replay after command invalidation and exact restoration");
    std::puts("PASS: authenticated source remaining/count/mode and ratio readers; missing owners/parameters preserve output, native zero/NaN/infinity and read-only field snapshots");
    std::puts("PASS:64 actual PS input-prefix oracle cases vs PC native physical bit31/fallback203, both input channels/mode and flag08; read-only snapshots");
}
