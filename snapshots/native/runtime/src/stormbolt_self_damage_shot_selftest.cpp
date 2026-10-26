#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_self_damage_shot.h"
#include "stormbolt_power_cell_timer.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace stormslinger;
namespace {
template<std::size_t N>struct Buffer {std::array<unsigned char,N> bytes{};};
Buffer<0x14000> g_actor;
Buffer<0x2860> g_weapon;
Buffer<0x1770> g_parameter;
Buffer<0x1500> g_skill;
Buffer<0xB00> g_resource;
Buffer<0x200> g_status,g_health,g_damage,g_loop;
std::uintptr_t parameter_type{},skill_type{},resource_type{},status_type{},health_type{},damage_type{},loop_type{};
void* resource_pointer=g_resource.bytes.data();
int g_primary{},g_sound{};
std::array<float,3> g_defaults{};
StormboltPowerCellStore<1> g_statuses;
StormboltPowerCellLoopStore<1> g_loops;
StormboltPowerCellToken g_token;
std::vector<unsigned> g_calls;
float g_last_amount{};
std::int32_t g_skill_value=1;
bool g_invalidate_on_effect=false;
template<class O,class T>void put(O& o,std::size_t at,T value) {
    assert(at+sizeof(value)<=o.bytes.size());std::memcpy(o.bytes.data()+at,&value,sizeof(value));
}
std::int32_t query(void* a,std::uint32_t id,std::uint32_t sub,std::int32_t fallback) {
    assert(a==g_actor.bytes.data() && id==0xD1 && sub==0 && fallback==0);return g_skill_value;
}
void adjust(void* d,float amount,float scale,bool a,bool b) {
    assert(d==g_damage.bytes.data() && scale==1 && a && !b);
    StormboltPowerCellState state{};assert(g_statuses.snapshot(g_token,state));
    g_last_amount=amount;g_calls.push_back(4);
}
void prompt(void* a,std::uint32_t id,bool removal,std::int32_t value) {
    assert(a==g_actor.bytes.data() && id==0x1D3 && !removal && value==-1);
    StormboltPowerCellState state{};assert(g_statuses.snapshot(g_token,state) && state.count==4);
    g_calls.push_back(1);
}
void* container(void* s){assert(s==&g_sound);return s;}
bool play(void* c,std::int32_t category,const char* name,StormboltNamedEventParameters*) {
    assert(c==&g_sound && category==-1 && std::strcmp(name,"PLAY_PL_BODY_068_DMG_R_LP")==0);
    g_calls.push_back(2);return false; // Native PLAY result does not cancel subsequent work.
}
void* equipment(void* p,std::uint32_t slot,std::uint32_t group,std::uint32_t record,bool post) {
    assert(p==&g_primary && slot==1 && group==0x7D0 && record==1 && post);
    g_calls.push_back(3);
    if(g_invalidate_on_effect)put(g_actor,0xC,std::uint8_t{0});
    return nullptr;
}
StormboltSelfDamageShotBindings setup(bool powered,std::uint32_t count) {
    g_actor={};g_weapon={};g_parameter={};g_skill={};g_resource={};g_status={};g_health={};g_damage={};g_loop={};
    g_calls.clear();g_last_amount=0;g_skill_value=1;g_invalidate_on_effect=false;
    put(g_weapon,0x9B0,g_actor.bytes.data());put(g_actor,0xC,std::uint8_t{0xE});
    put(g_actor,0x60,std::uint8_t(powered));put(g_actor,0x7810,g_parameter.bytes.data());
    put(g_actor,0x7D20,g_status.bytes.data());put(g_status,0,&status_type);put(g_status,8,g_actor.bytes.data());
    put(g_parameter,0,&parameter_type);put(g_skill,0,&skill_type);put(g_skill,0x141C,std::int32_t{1});
    put(g_actor,0x126D8,g_skill.bytes.data());put(g_resource,0,&resource_type);
    put(g_resource,0xA8C,std::uint32_t{4});put(g_resource,0xA94,10.0f);
    put(g_actor,0x7630,g_health.bytes.data());put(g_health,0,&health_type);put(g_health,0x64,200.0f);
    put(g_actor,0x7658,g_damage.bytes.data());put(g_damage,0,&damage_type);
    put(g_actor,0x126C8,g_loop.bytes.data());put(g_actor,0x13D58,&g_primary);put(g_actor,0xA18,&g_sound);
    put(g_loop,0,&loop_type);put(g_loop,0x60,g_actor.bytes.data());put(g_loop,0xF0,g_actor.bytes.data());
    for(unsigned i=0;i<9;++i) {put(g_parameter,0x1614+40*i,float(6+i));put(g_parameter,0x1618+40*i,float(16+i));}
    g_token=g_statuses.initialize(reinterpret_cast<std::uintptr_t>(g_status.bytes.data()));assert(g_token);
    assert(g_statuses.transact(g_token,[&](auto& s) noexcept {s.count=count;return true;}));
    assert(g_loops.initialize(reinterpret_cast<std::uintptr_t>(g_loop.bytes.data())));
    StormboltSelfDamageShotBindings api{};
    api.parameter_vtable=reinterpret_cast<std::uintptr_t>(&parameter_type);
    api.damage_vtable=reinterpret_cast<std::uintptr_t>(&damage_type);
    api.health_vtable=reinterpret_cast<std::uintptr_t>(&health_type);
    api.loop.component_vtable=reinterpret_cast<std::uintptr_t>(&loop_type);
    api.loop.container_at=container;api.loop.native_first=g_defaults.data();api.loop.native_second=g_defaults.data();
    api.loop.inputs={reinterpret_cast<std::uintptr_t>(&skill_type),reinterpret_cast<std::uintptr_t>(&status_type),
        reinterpret_cast<std::uintptr_t>(&resource_pointer),reinterpret_cast<std::uintptr_t>(&resource_type),0};
    api.loop.query=query;api.loop.request=play;api.adjust=adjust;api.prompt=prompt;api.equipment=equipment;
    return api;
}
}
int main() {
    for(unsigned stage=0;stage<9;++stage) {
        auto api=setup(false,0);assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),stage,g_statuses,g_loops,api));
        assert(g_last_amount==float(6+stage) && g_calls==std::vector<unsigned>{4});
        api=setup(true,0);assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),stage,g_statuses,g_loops,api));
        assert(g_last_amount==(stage==8?100.0f:float(16+stage)));
        assert(g_calls==(stage==8?std::vector<unsigned>{1,2,3,4}:std::vector<unsigned>{4}));
    }
    auto api=setup(true,4);assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==14 && g_calls==std::vector<unsigned>{4}); // Threshold equality uses normal branch.
    api=setup(true,5);assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==100 && g_calls==std::vector<unsigned>{4}); // Above threshold: no promotion.
    api=setup(false,0);put(g_parameter,0x1614,0.0f);
    assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),0,g_statuses,g_loops,api) && g_calls.empty());
    api=setup(true,0);assert(g_statuses.retire(g_token));
    assert(!apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api) && g_calls.empty());
    // A disabled skill must not require a power resource or promote stale state.
    for(const auto skill_value:{0,-1}) {
        api=setup(true,0);g_skill_value=skill_value;
        api.loop.inputs.resource_slot=0;
        StormboltPowerCellState before{};
        assert(g_statuses.snapshot(g_token,before));
        assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
        assert(g_last_amount==14 && g_calls==std::vector<unsigned>{4});
        StormboltPowerCellState state{};
        assert(g_statuses.snapshot(g_token,state) && state.count==before.count && state.timer==before.timer);
    }
    // The half-health branch reads live current health, not max or cached HP.
    api=setup(true,5);put(g_health,0x60,200.0f);put(g_health,0x64,81.0f);
    assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==40.5f && g_calls==std::vector<unsigned>{4});
    // Native effect callbacks can invalidate the actor before health dispatch.
    api=setup(true,0);g_invalidate_on_effect=true;
    assert(!apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==0 && g_calls==std::vector<unsigned>({1,2,3}));
    // Same live generation: conditional overload changes the next shot's branch.
    api=setup(true,0);put(g_parameter,0x1614+40*8,999.0f);
    put(g_health,0x64,81.0f);
    assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==40.5f && g_calls==std::vector<unsigned>({1,2,3,4}));
    g_calls.clear();
    assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==999.0f && g_calls==std::vector<unsigned>{4});
    assert(g_statuses.transact(g_token,[](auto& state) noexcept {
        return advance_stormbolt_power_cell_timer(state.count,state.timer,true,10.0f)==
            StormboltPowerCellTimerResult::expired;
    }));
    g_calls.clear();
    assert(apply_stormbolt_shot_self_damage(g_weapon.bytes.data(),8,g_statuses,g_loops,api));
    assert(g_last_amount==40.5f && g_calls==std::vector<unsigned>({1,2,3,4}));
    std::puts("PASS: shot self-damage stages, live half-current-health, threshold gates, disabled skill without resource, ordered effects and callback invalidation; native calls are fixtures");
}
