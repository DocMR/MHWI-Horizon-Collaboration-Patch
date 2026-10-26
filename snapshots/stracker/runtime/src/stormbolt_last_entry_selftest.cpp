#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_last_entry.h"
#include "stormbolt_last_update.h"
#include "stormbolt_last_session.h"
#include "stormbolt_update_gate.h"
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_cleanup_decay.h"
#include "stormbolt_cleanup_route.h"
#include "stormbolt_decay_effect.h"
#include "stormbolt_decay_prefix.h"
#include "stormbolt_local_decay_timer.h"
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_effect_alpha.h"
#include "stormbolt_effect_color.h"
#include "stormbolt_effect_fade.h"
#include "stormbolt_decay_route.h"
#include "stormbolt_input_window.h"
#include "stormbolt_input_trigger.h"
#include "stormbolt_local_trigger_route.h"
#include "stormbolt_local_input_prefix.h"
#include "stormbolt_local_update.h"
#include "stormbolt_stage_selection.h"
#include "stormbolt_inactive_update.h"
#include "stormbolt_stage_update.h"
#include "stormbolt_weapon_update.h"
#include "stormbolt_weapon_update_hook.h"
#include "stormbolt_self_damage_amount.h"
#include "stormbolt_self_damage_adjustment.h"
#include "stormbolt_self_damage_branch.h"
#include "stormbolt_self_damage_effect.h"
#include "stormbolt_power_cell_query.h"
#include "stormbolt_power_cell_parameters.h"
#include "stormbolt_power_cell_timer.h"
#include "stormbolt_power_cell_multiplier.h"
#include "stormbolt_power_cell_store.h"
#include "stormbolt_power_cell_lifecycle.h"
#include "stormbolt_power_cell_multiplier_adapter.h"
#include "stormbolt_power_cell_self_damage_state.h"
#include "stormbolt_power_cell_prompt.h"
#include "stormbolt_named_event_parameters.h"
#include "stormbolt_power_cell_sound.h"
#include "stormbolt_power_cell_loop_state.h"
#include "stormbolt_power_cell_loop_adapter.h"
#include "stormbolt_loop_native_context.h"
#include "stormbolt_loop_skill_gate.h"
#include "stormbolt_loop_update_inputs.h"
#include "stormbolt_loop_audio_update.h"
#include "stormbolt_loop_audio_dispatch.h"
#include "stormbolt_power_cell_visual_state.h"
#include "stormbolt_power_cell_event_growth.h"
#include "stormbolt_power_cell_event_eligibility.h"
#include "stormbolt_power_cell_event_eligibility_adapter.h"
#include "stormbolt_power_cell_timer_adapter.h"
#include <string_view>
#include "stormbolt_attachment_blink.h"
#include "stormbolt_material_color.h"
#include "stormbolt_attachment_color.h"
#include "stormbolt_attachment_update.h"
#include "stormbolt_attachment_gate.h"
#include "stormbolt_high_stage.h"
#include "stormbolt_remote_effect.h"
#include <stdexcept>
#include <cassert>
#include <cstdio>
#include <vector>
#include <utility>
using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes=std::array<std::byte,N>;
Bytes<0x38> g_action;
Bytes<0x9000> g_actor,g_other;
Bytes<0x2860> g_weapon;
Bytes<0x176C> g_parameter,g_other_parameter;
StormboltInstanceStore<1> g_store;
StormboltInstanceToken g_token;
std::vector<unsigned> g_calls;
unsigned g_scenario=0;
__declspec(noinline) void power_cell_reset_fault(unsigned& calls) {
    ++calls; if(g_scenario==99) throw std::runtime_error("fixture");
}
template<class T> void put(void* p,std::size_t o,T v) { std::memcpy(static_cast<std::byte*>(p)+o,&v,sizeof(v)); }
template<class T> T get(const void* p,std::size_t o) { T v{}; std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v)); return v; }
template<class T> T action_field(std::size_t offset) {
    StormboltStageState state{};
    if(g_store.snapshot(g_token,state) && state.action_owner==reinterpret_cast<std::uintptr_t>(g_action.data()))
        return static_cast<T>(offset==0x30?state.action_phase:state.action_fired);
    return get<T>(g_action.data(),offset);
}
void mode(void* p,std::uint32_t value) {
    assert(p==g_actor.data() && value==0 && get<std::uint32_t>(p,0x82D0)==1);
    assert(action_field<std::uint32_t>(0x30)==9); g_calls.push_back(1);
    if(g_scenario==1) put(g_action.data(),0x28,g_other.data());
}
bool angle(void* p,float value,std::uint32_t kind) {
    assert(p==(g_scenario==1 ? g_other.data() : g_actor.data()) && value==1.75f && kind==2);
    assert(action_field<std::uint32_t>(0x30)==0); g_calls.push_back(2);
    StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
    if(g_scenario==2) {
        put(g_action.data(),0x28,g_other.data());
        put(g_weapon.data(),0x9B0,g_other.data());
    }
    if(g_scenario==4) assert(g_store.retire(g_token));
    return false; // Native return is ignored, not a new success gate.
}
void motion(void* p,std::uint32_t id,std::uint32_t arg) {
    assert(p==((g_scenario==1 || g_scenario==2) ? g_other.data() : g_actor.data()));
    assert(id==0xC0F5 && arg==0); g_calls.push_back(3);
    StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
    assert(state.decay_timer==(g_scenario==2 ? 0.75f : 0.5f));
    // A newly owned PC sidecar starts cleared; direct PS-sized fixtures retain
    // their initial byte until entry's final write. Neither writes past PC30.
    assert(action_field<std::uint8_t>(0x34)==
        (state.action_owner==reinterpret_cast<std::uintptr_t>(g_action.data())?0:1));
    if(g_scenario==3) put(g_action.data(),0x28,g_other.data());
    if(g_scenario==5) assert(g_store.retire(g_token));
    if(g_scenario==6) put(g_action.data(),0x28,static_cast<void*>(nullptr));
}
Bytes<0x200> g_event;
float g_delta=0;
unsigned g_fires=0,g_effects=0,g_queries=0,g_flags=0;
bool g_completed=false,g_effect_ok=true,g_fire_throw=false;
void* update_event(void* p) { assert(p==g_actor.data()); return g_event.data(); }
float update_difference(void* p) { assert(p==g_actor.data()); return g_delta; }
const void* update_origin(void* p) { assert(p==g_actor.data()); return p; }
void update_fire(void* p,const void* point,bool a,std::uint32_t b,bool c) {
    assert(p==g_weapon.data() && point==g_actor.data() && a && b==0xffffffff && c);
    StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
    assert(!(get<std::uint32_t>(g_event.data(),0x28)&1)); ++g_fires;
    if(g_fire_throw) throw std::runtime_error("fire");
}
void update_flag(void* p,std::uint32_t id) { assert(p==g_actor.data() && (id==0x16D || id==0xDB)); ++g_flags; }
bool update_effect(void* p) {
    assert(p==g_actor.data() && !(get<std::uint32_t>(g_event.data(),0x28)&0x40000000));
    ++g_effects; return g_effect_ok;
}
bool update_complete(void* p) { assert(p==g_actor.data()); ++g_queries; return g_completed; }
void setup(std::uint32_t stage,bool local,unsigned which=0) {
    g_action.fill(std::byte{0}); g_actor.fill(std::byte{0}); g_other.fill(std::byte{0});
    g_weapon.fill(std::byte{0}); g_parameter.fill(std::byte{0}); g_other_parameter.fill(std::byte{0});
    g_scenario=which; g_calls.clear();
    put(g_action.data(),0x28,g_actor.data()); put(g_action.data(),0x30,std::uint32_t{9});
    put(g_action.data(),0x34,std::uint8_t{1});
    for(auto* p : {g_actor.data(),g_other.data()}) {
        put(p,0xC,std::uint8_t{2}); put(p,0x76B0,g_weapon.data());
        put(p,0x8938,std::uint8_t(local));
    }
    put(g_actor.data(),0x7810,g_parameter.data()); put(g_other.data(),0x7810,g_other_parameter.data());
    put(g_weapon.data(),0xC,std::uint8_t{2}); put(g_weapon.data(),0x9F8,std::uint32_t{13});
    put(g_weapon.data(),0x9FC,std::uint32_t{239}); put(g_weapon.data(),0x9B0,g_actor.data());
    put(g_parameter.data(),0x818,1.75f);
    for(unsigned i=0;i<9;++i) {
        put(g_parameter.data(),0x1608+40*i,10.0f+i);
        put(g_other_parameter.data(),0x1608+40*i,20.0f+i);
    }
    put(g_parameter.data(),0x1768,0.5f); put(g_other_parameter.data(),0x1768,0.75f);
    g_token=g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data())); assert(g_token);
    assert(g_store.transact(g_token,[stage](auto& s) noexcept { s={stage,77,123.0f,456.0f}; return true; }));
}
}
int main() {
    {
        StormboltPowerCellState state{2,13.0f};
        StormboltPowerCellParameters parameters{};parameters.threshold=4;parameters.duration=150.0f;
        assert(advance_stormbolt_power_cell_accepted_event(state,parameters)==StormboltPowerCellGrowthResult::below_threshold);
        assert(state.count==3 && state.timer==13.0f);
        update_stormbolt_power_cell_event_eligibility(state,true,true,1,4);
        assert(state.event_eligible);
        assert(advance_stormbolt_power_cell_accepted_event(state,parameters)==StormboltPowerCellGrowthResult::reached_threshold);
        assert(state.count==4 && state.timer==150.0f);
        update_stormbolt_power_cell_event_eligibility(state,true,true,1,4);
        assert(!state.event_eligible);
        assert(advance_stormbolt_power_cell_accepted_event(state,parameters)==StormboltPowerCellGrowthResult::unchanged);
        assert(state.count==4 && state.timer==150.0f);
        state.count=5;
        update_stormbolt_power_cell_event_eligibility(state,true,true,1,4);
        assert(state.event_eligible); // Native producer is not a less-than gate.
        update_stormbolt_power_cell_event_eligibility(state,true,true,-1,4);
        assert(!state.event_eligible);
        update_stormbolt_power_cell_event_eligibility(state,false,true,1,4);
        assert(!state.event_eligible);
        update_stormbolt_power_cell_event_eligibility(state,true,false,1,4);
        assert(!state.event_eligible);
        parameters.threshold=0;
        assert(advance_stormbolt_power_cell_accepted_event(state,parameters)==StormboltPowerCellGrowthResult::unchanged);
    }
    {
        StormboltPowerCellLoopState state{};
        state.growth=true;
        update_stormbolt_power_cell_non_threshold_visual(state,2.0f);
        assert(state.visual_phase==1 && state.visual_timer==0.7f);
        assert(stormbolt_power_cell_visual_scale(state,false)==1000.0f);
        state.visual_timer=0.1f;
        update_stormbolt_power_cell_non_threshold_visual(state,2.0f);
        assert(state.visual_phase==1 && state.visual_timer==0.7f);
        state.growth=false;
        update_stormbolt_power_cell_non_threshold_visual(state,0.7f);
        assert(state.visual_phase==0 && state.visual_timer==0.0f);
        state.visual_phase=1;state.visual_timer=1.0f;
        update_stormbolt_power_cell_non_threshold_visual(state,std::numeric_limits<float>::quiet_NaN());
        assert(state.visual_phase==1 && std::isnan(state.visual_timer));
        reset_stormbolt_power_cell_threshold_visual(state);
        assert(state.visual_phase==0 && state.visual_timer==-1.0f && state.timer==0.0f);
        assert(stormbolt_power_cell_visual_scale(state,false)==400.0f);
        assert(stormbolt_power_cell_visual_scale(state,true)==400.0f);
        state.timer=5.0f;
        assert(stormbolt_power_cell_visual_scale(state,true)==700.0f);
    }
    {
        std::array<std::byte,0x126E0> owner{};
        std::array<std::byte,0x15A8> skills{};
        std::array<std::byte,0xA10> status{};
        std::array<std::byte,0xA98> resource{};
        put(owner.data(),0x60,std::uint8_t{1});
        put(owner.data(),0x126D8,static_cast<void*>(skills.data()));
        put(owner.data(),0x7D20,static_cast<void*>(status.data()));
        put(skills.data(),0,std::uintptr_t{1});
        put(skills.data(),0x141C,std::int32_t{3});
        put(status.data(),0,std::uintptr_t{2});
        put(status.data(),8,static_cast<void*>(owner.data()));
        put(resource.data(),0,std::uintptr_t{3});
        put(resource.data(),0xA8C,std::uint32_t{9});
        std::uintptr_t slot=reinterpret_cast<std::uintptr_t>(resource.data());
        StormboltPowerCellStore<1> store;
        const auto token=store.initialize(reinterpret_cast<std::uintptr_t>(status.data()));
        assert(store.transact(token,[](auto& state) noexcept { state.count=7;return true; }));
        const StormboltLoopInputBindings bindings{1,2,reinterpret_cast<std::uintptr_t>(&slot),3};
        const auto query=+[](void*,std::uint32_t,std::uint32_t,std::int32_t) { return 1; };
        static unsigned eligibilityFlags=0;
        const auto flag=+[](void*,std::uint32_t value) {assert(value==0x98);++eligibilityFlags;};
        assert(update_stormbolt_power_cell_event_eligibility_from_status(status.data(),store,bindings,query,flag));
        StormboltPowerCellState eligibilityState{};
        assert(store.snapshot(token,eligibilityState) && eligibilityState.event_eligible && eligibilityFlags==1);
        assert(store.transact(token,[](auto& s) noexcept {s.count=9;return true;}));
        assert(update_stormbolt_power_cell_event_eligibility_from_status(status.data(),store,bindings,query,flag));
        assert(store.snapshot(token,eligibilityState) && !eligibilityState.event_eligible && eligibilityFlags==1);
        assert(store.transact(token,[](auto& s) noexcept {s.count=7;return true;}));
        static StormboltPowerCellStore<1>* timerStore=nullptr;
        static StormboltPowerCellToken timerToken{};
        static unsigned timerQueries=0,timerPrompts=0;
        timerStore=&store;timerToken=token;
        const auto timerQuery=+[](void*,std::uint32_t,std::uint32_t,std::int32_t) {
            StormboltPowerCellState s{};
            assert(timerStore->snapshot(timerToken,s) && s.count==0 && s.timer==-1.0f);
            ++timerQueries;return 1;
        };
        const auto timerPrompt=+[](void*,std::uint32_t id,bool removal,std::int32_t value) {
            assert(id==0x1D3 && removal && value==-1 && timerQueries==1);++timerPrompts;
        };
        put(owner.data(),0x8938,std::uint8_t{1});
        assert(store.transact(token,[](auto& s) noexcept {s={4,0.5f,true};return true;}));
        assert(advance_stormbolt_power_cell_timer_from_status(status.data(),0.25f,store,bindings,timerQuery,timerPrompt));
        assert(store.snapshot(token,eligibilityState) && eligibilityState.timer==0.25f && timerQueries==0);
        put(owner.data(),0x8938,std::uint8_t{0});
        assert(advance_stormbolt_power_cell_timer_from_status(status.data(),1.0f,store,bindings,timerQuery,timerPrompt));
        assert(store.snapshot(token,eligibilityState) && eligibilityState.timer==0.25f && timerQueries==0);
        put(owner.data(),0x8938,std::uint8_t{1});
        assert(advance_stormbolt_power_cell_timer_from_status(status.data(),1.0f,store,bindings,timerQuery,timerPrompt));
        assert(store.snapshot(token,eligibilityState) && eligibilityState.count==0 && eligibilityState.timer==-1.0f && eligibilityState.event_eligible);
        assert(timerQueries==1 && timerPrompts==1);
        assert(store.transact(token,[](auto& s) noexcept {s={7,-1.0f,false};return true;}));
        StormboltPowerCellLoopUpdateInputs input{};
        assert(read_stormbolt_loop_update_inputs(owner.data(),store,bindings,query,input));
        assert(input.count==7 && input.threshold==9 && input.resource_count==1);
        std::array<std::byte,0x580> component{};
        std::array<std::byte,0x180> actor{};
        std::array<std::byte,0x80> sound{};
        put(component.data(),0,std::uintptr_t{4});
        put(component.data(),0x60,static_cast<void*>(owner.data()));
        put(component.data(),0xF0,static_cast<void*>(actor.data()));
        put(owner.data(),0x68,1.0f);
        put(owner.data(),0xA18,static_cast<void*>(sound.data()));
        put(actor.data(),0xC,std::uint8_t{0xE});
        const std::array<float,3> defaults{{0,0,1}};
        const auto containerAt=+[](void* p)->void* { return static_cast<std::byte*>(p)+0x60; };
        static unsigned soundCalls=0;
        static std::vector<std::uint32_t> componentEvents;
        const auto soundRequest=+[](void*,std::int32_t category,const char* name,StormboltNamedEventParameters* p) {
            assert(category==-1 && p->flags==0x12);
            assert(std::string_view(name)=="STOP_PL_BODY_068_DMG_R_LP");
            componentEvents.push_back(0);++soundCalls;return false;
        };
        StormboltPowerCellLoopStore<1> loops;
        const auto loopToken=loops.initialize(reinterpret_cast<std::uintptr_t>(component.data()));
        assert(loops.transact(loopToken,[](auto& state) noexcept { state={5.0f,true};return true; }));
        const StormboltLoopAudioUpdateBindings audio{4,containerAt,defaults.data(),defaults.data(),bindings,query,soundRequest};
        assert(update_stormbolt_loop_audio_from_component(component.data(),loops,store,audio));
        StormboltPowerCellLoopState loopState{};
        assert(loops.snapshot(loopToken,loopState) && loopState.timer==-1.0f && !loopState.active && soundCalls==1);
        StormboltLoopAudioDispatch<1,1> dispatch(loops,store,audio);
        const auto previous=stormbolt_loop_update_dispatch_target;
        assert(dispatch.publish());
        assert(loops.transact(loopToken,[](auto& state) noexcept { state={5.0f,true};return true; }));
        stormbolt_loop_update_dispatch_target(component.data());
        assert(loops.snapshot(loopToken,loopState) && !loopState.active && soundCalls==2);
        assert(dispatch.withdraw() && stormbolt_loop_update_dispatch_target==previous);
        std::array<std::byte,0x448> material{};
        put(owner.data(),0x2B8,std::uint32_t{1});
        const auto materialAt=+[](void* owner,std::uint32_t index)->void* {
            assert(index==0);
            void* result=nullptr;std::memcpy(&result,static_cast<std::byte*>(owner)+0x2B0,sizeof(result));return result;
        };
        put(owner.data(),0x2B0,static_cast<void*>(material.data()));
        const auto convert=+[](const float* source,float* output)->float* {
            std::memcpy(output,source,16);return output;
        };
        const auto scalar=+[](void*,std::uint32_t hash,float) {componentEvents.push_back(hash);};
        const StormboltMaterialColorApi materialApi{+[](void*,std::uint32_t hash,const float*) {componentEvents.push_back(hash);}};
        const StormboltLoopComponentUpdateBindings combined{audio,materialAt,convert,scalar,materialApi};
        StormboltLoopComponentDispatch<1,1> combinedDispatch(loops,store,combined);
        assert(combinedDispatch.publish());
        componentEvents.clear();
        assert(loops.transact(loopToken,[](auto& s) noexcept {s={5.0f,true};s.growth=true;return true;}));
        stormbolt_loop_update_dispatch_target(component.data());
        assert((componentEvents==std::vector<std::uint32_t>{0,0xA5FC8881u,0xE8C621B1u,0x58662D14u}));
        assert(loops.snapshot(loopToken,loopState) && !loopState.growth && loopState.visual_phase==1);
        assert(store.transact(token,[](auto& s) noexcept {s.count=9;return true;}));
        assert(loops.transact(loopToken,[](auto& s) noexcept {s={0.5f,true};s.growth=true;return true;}));
        componentEvents.clear();
        stormbolt_loop_update_dispatch_target(component.data());
        assert((componentEvents==std::vector<std::uint32_t>{0xA5FC8881u,0xE8C621B1u,0x58662D14u,0}));
        assert(loops.snapshot(loopToken,loopState) && !loopState.active && loopState.timer==-0.5f && !loopState.growth);
        put(component.data(),0x570,std::uint8_t{1});
        assert(loops.transact(loopToken,[](auto& s) noexcept {s={5.0f,true};s.growth=true;return true;}));
        componentEvents.clear();stormbolt_loop_update_dispatch_target(component.data());
        assert(componentEvents.empty());
        assert(loops.snapshot(loopToken,loopState) && loopState.timer==4.0f && !loopState.growth);
        assert(combinedDispatch.withdraw() && stormbolt_loop_update_dispatch_target==previous);
        assert(store.retire(token));
        assert(!update_stormbolt_loop_audio_from_component(component.data(),loops,store,audio));
        assert(soundCalls==4);
        assert(!read_stormbolt_loop_update_inputs(owner.data(),store,bindings,query,input));
        assert(input.count==7 && input.threshold==9);
        put(owner.data(),0x60,std::uint8_t{0});
        assert(read_stormbolt_loop_update_inputs(owner.data(),store,{},nullptr,input));
        assert(!input.owner_flag && input.count==0 && input.threshold==0);
    }
    {
        std::array<std::byte,0x126E0> owner{};
        std::array<std::byte,0x15A8> skills{};
        put(owner.data(),0x60,std::uint8_t{1});
        put(owner.data(),0x126D8,static_cast<void*>(skills.data()));
        put(skills.data(),0,std::uintptr_t{0x1234});
        put(skills.data(),0x141C,std::int32_t{7});
        static unsigned calls=0;
        const auto query=+[](void*,std::uint32_t id,std::uint32_t value,std::int32_t fallback) {
            assert(id==0xD1 && value==0 && fallback==0);++calls;return -2;
        };
        StormboltPowerCellLoopUpdateInputs input{false,false,0,3,4};
        assert(read_stormbolt_loop_skill_gate(owner.data(),0x1234,query,input));
        assert(input.owner_flag && input.resource_index_present && input.resource_count==-2);
        assert(input.count==3 && input.threshold==4 && calls==1);
        put(skills.data(),0x141C,std::int32_t{0});
        assert(read_stormbolt_loop_skill_gate(owner.data(),0x1234,query,input));
        assert(!input.resource_index_present && input.resource_count==0 && calls==1);
        put(owner.data(),0x60,std::uint8_t{0});
        assert(read_stormbolt_loop_skill_gate(owner.data(),0,nullptr,input));
        assert(!input.owner_flag && calls==1);
    }
    {
        std::array<std::byte,0x100> component{};
        std::array<std::byte,0xB00> owner{};
        std::array<std::byte,0x180> actor{};
        std::array<std::byte,0x80> sound{};
        put(component.data(),0,std::uintptr_t{0x1234});
        put(component.data(),0x60,static_cast<void*>(owner.data()));
        put(component.data(),0xF0,static_cast<void*>(actor.data()));
        put(owner.data(),0x68,-2.0f);
        put(owner.data(),0xA18,static_cast<void*>(sound.data()));
        put(actor.data(),0xC,std::uint8_t{0xE});
        const std::array<float,3> position{{1,2,3}},first{{4,5,6}},second{{7,8,9}};
        put(actor.data(),0x160,position);
        const auto containerAt=+[](void* p)->void* { return static_cast<std::byte*>(p)+0x60; };
        StormboltLoopNativeContext result{};
        assert(read_stormbolt_loop_native_context(component.data(),0x1234,containerAt,first.data(),second.data(),result));
        assert(result.owner==owner.data() && result.actor==actor.data() && result.container==sound.data()+0x60);
        assert(result.actor_available && result.step==-2.0f && result.position==position);
        assert(result.defaults[0]==first && result.defaults[1]==second && result.defaults[2]==first);
        assert(!read_stormbolt_loop_native_context(component.data(),0x5678,containerAt,first.data(),second.data(),result));
        assert(result.actor_available && result.position==position);
        put(actor.data(),0xC,std::uint8_t{0});
        assert(read_stormbolt_loop_native_context(component.data(),0x1234,nullptr,nullptr,nullptr,result));
        assert(!result.actor_available && !result.container && result.step==-2.0f);
    }
    {
        static StormboltPowerCellLoopStore<1> store;
        static StormboltPowerCellLoopToken token{};
        static bool rebuild=false;
        static unsigned calls=0;
        const auto address=reinterpret_cast<std::uintptr_t>(g_actor.data());
        token=store.initialize(address);
        const auto request=+[](void*,std::int32_t,const char*,StormboltNamedEventParameters*) {
            StormboltPowerCellLoopState state{};
            assert(store.snapshot(token,state) && state.timer==5.0f);
            ++calls;
            if (rebuild) store.initialize(token.address);
            return false;
        };
        const std::array<std::array<float,3>,3> defaults{};
        assert(start_stormbolt_power_cell_loop_request(store,token,g_weapon.data(),g_actor.data(),defaults,request));
        StormboltPowerCellLoopState state{};
        assert(store.snapshot(token,state) && state.active && calls==1);
        rebuild=true;
        assert(!start_stormbolt_power_cell_loop_request(store,token,g_weapon.data(),g_actor.data(),defaults,request));
        StormboltPowerCellLoopToken fresh{};
        assert(store.token_for_live_address(address,fresh));
        assert(store.snapshot(fresh,state) && !state.active && state.timer==0.0f && calls==2);
        assert(!start_stormbolt_power_cell_loop_request(store,token,g_weapon.data(),g_actor.data(),defaults,request));
        assert(calls==2);
        const auto old=token;
        token=fresh;
        assert(store.transact(token,[](auto& s) noexcept { s={5.0f,true};return true; }));
        const auto stopRequest=+[](void*,std::int32_t,const char* name,StormboltNamedEventParameters* p) {
            StormboltPowerCellLoopState during{};
            assert(store.snapshot(token,during) && during.active && during.timer==-1.0f);
            assert(std::string_view(name)=="STOP_PL_BODY_068_DMG_R_LP" && p->flags==0x12);
            ++calls;
            return false;
        };
        const auto tick=[](auto& s,bool available,auto& stop) noexcept {
            update_stormbolt_power_cell_loop_timer(s,6.0f,available,stop);
        };
        const std::array<float,3> position{};
        assert(update_stormbolt_power_cell_loop_request(store,token,tick,g_weapon.data(),g_actor.data(),defaults,position,stopRequest));
        assert(store.snapshot(token,state) && !state.active && state.timer==-1.0f && calls==3);
        assert(!update_stormbolt_power_cell_loop_request(store,old,tick,g_weapon.data(),g_actor.data(),defaults,position,stopRequest));
        assert(calls==3);
        assert(store.transact(token,[](auto& s) noexcept { s={5.0f,true};return true; }));
        StormboltLoopNativeContext context{};
        context.container=g_weapon.data();context.actor=g_actor.data();context.actor_available=true;
        context.step=1.0f;
        const StormboltPowerCellLoopUpdateInputs input{true,true,1,2,4};
        assert(process_stormbolt_power_cell_loop_audio(store,token,context,input,stopRequest));
        assert(store.snapshot(token,state) && !state.active && state.timer==-1.0f && calls==4);
    }
    {
        StormboltPowerCellLoopStore<1> store;
        const auto address=reinterpret_cast<std::uintptr_t>(g_actor.data());
        const auto old=store.initialize(address);
        assert(store.transact(old,[](auto& s) noexcept { s={5.0f,true}; return true; }));
        assert(store.retire(old));
        const auto fresh=store.initialize(address);
        StormboltPowerCellLoopState state{};
        assert(fresh.generation!=old.generation && !store.snapshot(old,state));
        assert(store.snapshot(fresh,state) && state.timer==0.0f && !state.active);
        assert(state.visual_timer==-1.0f && !state.visual_active && !state.growth && state.visual_phase==0);
    }
    {
        unsigned calls=0;
        const auto stop=[&]() { ++calls; return false; };
        unsigned plays=0;
        const auto play=[&]() { ++plays; return false; };
        StormboltPowerCellLoopState starting{};
        assert(!start_stormbolt_power_cell_loop(starting,false,play));
        assert(starting.timer==5.0f && !starting.active && plays==0);
        assert(start_stormbolt_power_cell_loop(starting,true,play));
        assert(starting.timer==5.0f && starting.active && plays==1);
        starting.timer=-1.0f;
        assert(!start_stormbolt_power_cell_loop(starting,false,play));
        assert(starting.timer==5.0f && starting.active && plays==1);
        StormboltPowerCellLoopState state{5.0f,true};
        assert(!update_stormbolt_power_cell_loop_timer(state,5.0f,true,stop));
        assert(state.timer==0.0f && state.active && calls==0);
        assert(!update_stormbolt_power_cell_loop_timer(state,1.0f,true,stop));
        state={5.0f,true};
        assert(update_stormbolt_power_cell_loop_timer(state,6.0f,true,stop));
        assert(state.timer==-1.0f && !state.active && calls==1);
        state={5.0f,true};
        assert(!update_stormbolt_power_cell_loop_timer(state,6.0f,false,stop));
        assert(state.timer==-1.0f && state.active && calls==1);
        state={5.0f,false};
        assert(!update_stormbolt_power_cell_loop_timer(state,6.0f,true,stop));
        state={5.0f,true};
        assert(!update_stormbolt_power_cell_loop_timer(state,-1.0f,true,stop));
        assert(state.timer==6.0f);
        state={std::numeric_limits<float>::quiet_NaN(),true};
        assert(!update_stormbolt_power_cell_loop_timer(state,1.0f,true,stop));
        assert(state.active && calls==1);
        state={5.0f,true};
        assert(!update_stormbolt_power_cell_loop_non_equal_count(state,4,4,true,stop));
        assert(state.timer==5.0f && state.active);
        state={5.0f,true};
        assert(update_stormbolt_power_cell_loop_non_equal_count(state,1,4,true,stop));
        assert(state.timer==-1.0f && !state.active && calls==2);
        state={5.0f,true};
        assert(!update_stormbolt_power_cell_loop_non_equal_count(state,1,4,false,stop));
        assert(state.timer==-1.0f && state.active && calls==2);
        state={5.0f,true};
        assert(update_stormbolt_power_cell_loop_non_equal_count(state,2,4,true,stop));
        assert(!state.active && calls==3);
        for (unsigned mask=0;mask<8;++mask) {
            state={5.0f,true};
            const bool eligible=mask==7;
            const unsigned before=calls;
            assert(stop_stormbolt_power_cell_loop_for_resource_gate(
                state,(mask&1)!=0,(mask&2)!=0,(mask&4)?1:0,true,stop)==!eligible);
            assert(state.active==eligible && state.timer==5.0f);
            assert(calls==before+(eligible?0:1));
        }
        state={5.0f,true};
        assert(!stop_stormbolt_power_cell_loop_for_resource_gate(state,false,false,-1,false,stop));
        assert(state.active && state.timer==5.0f);
    }
    {
        static unsigned calls=0;
        static bool result=false;
        const auto request=+[](void* container,std::int32_t category,
                              const char* name,StormboltNamedEventParameters* p) {
            assert(container==g_weapon.data() && category==-1);
            assert(p->actor==g_actor.data());
            const bool stop=std::string_view(name)=="STOP_PL_BODY_068_DMG_R_LP";
            assert(stop || std::string_view(name)=="PLAY_PL_BODY_068_DMG_R_LP");
            assert(p->flags==(stop ? 0x12u : 0x10u));
            if (stop) assert(p->vectors[0][0]==9.0f);
            ++calls;
            return result;
        };
        const std::array<std::array<float,3>,3> defaults{};
        const std::array<float,3> position{{9.0f,8.0f,7.0f}};
        assert(!request_stormbolt_power_cell_loop_play(nullptr,g_actor.data(),defaults,request));
        assert(!request_stormbolt_power_cell_loop_play(g_weapon.data(),nullptr,defaults,request));
        assert(calls==0);
        assert(!request_stormbolt_power_cell_loop_play(g_weapon.data(),g_actor.data(),defaults,request));
        result=true;
        assert(request_stormbolt_power_cell_loop_stop(g_weapon.data(),g_actor.data(),defaults,position,request));
        assert(calls==2);
    }
    {
        const std::array<std::array<float,3>,3> defaults{{{{1.0f,2.0f,3.0f}},{{4.0f,5.0f,6.0f}},{{7.0f,8.0f,9.0f}}}};
        StormboltNamedEventParameters params{};
        params.flags=0xFF;
        assert(!build_stormbolt_actor_event_parameters(nullptr,defaults,params));
        assert(params.flags==0xFF);
        assert(build_stormbolt_actor_event_parameters(g_actor.data(),defaults,params));
        assert(params.actor==g_actor.data() && params.flags==0x10);
        for (std::size_t v=0;v<3;++v) {
            for (std::size_t n=0;n<3;++n) assert(params.vectors[v][n]==defaults[v][n]);
            assert(params.vectors[v][3]==0.0f);
        }
        for (auto value:params.callbacks) assert(value==0);
        const std::array<float,3> position{{-12.0f,34.0f,56.0f}};
        assert(build_stormbolt_stop_event_parameters(g_actor.data(),defaults,position,params));
        assert(params.actor==g_actor.data() && params.flags==0x12);
        for (std::size_t n=0;n<3;++n) assert(params.vectors[0][n]==position[n]);
        assert(params.vectors[0][3]==0.0f);
        for (std::size_t v=1;v<3;++v)
            for (std::size_t n=0;n<3;++n) assert(params.vectors[v][n]==defaults[v][n]);
        for (auto value:params.callbacks) assert(value==0);
        assert(!build_stormbolt_stop_event_parameters(nullptr,defaults,position,params));
        assert(params.flags==0x12 && params.actor==g_actor.data());
    }

    {
        static unsigned prompt_calls=0;
        static bool last_removal=false;
        const auto prompt=+[](void* actor,std::uint32_t record,bool removal,std::int32_t priority) {
            assert(actor==g_actor.data() && record==467 && priority==-1);
            ++prompt_calls;last_removal=removal;
        };
        assert(request_stormbolt_power_cell_prompt(g_actor.data(),false,prompt));
        assert(prompt_calls==1 && !last_removal);
        assert(request_stormbolt_power_cell_prompt(g_actor.data(),true,prompt));
        assert(prompt_calls==2 && last_removal);
        assert(!request_stormbolt_power_cell_prompt(nullptr,false,prompt));
        assert(!request_stormbolt_power_cell_prompt(g_actor.data(),false,nullptr));
        assert(prompt_calls==2);
    }
    {
        StormboltPowerCellStore<1> states;
        const auto token=states.initialize(0x4000);
        const StormboltPowerCellParameters parameters{4,2,150.0f};
        for(std::uint32_t count=0;count<6;++count) {
            assert(states.transact(token,[&](auto& next) noexcept {next.count=count;next.timer=7.0f;return true;}));
            const auto result=promote_stormbolt_power_cell_for_self_damage(states,token,true,8,parameters);
            assert(result==(count<4 ? StormboltPowerCellPromotion::promoted : StormboltPowerCellPromotion::not_required));
            StormboltPowerCellState state{};assert(states.snapshot(token,state));
            assert(state.count==(count<4 ? 4 : count) && state.timer==(count<4 ? 150.0f : 7.0f));
        }
        assert(promote_stormbolt_power_cell_for_self_damage(states,{},false,8,parameters)==StormboltPowerCellPromotion::not_required);
        assert(promote_stormbolt_power_cell_for_self_damage(states,{},true,7,parameters)==StormboltPowerCellPromotion::not_required);
        assert(states.retire(token));
        assert(promote_stormbolt_power_cell_for_self_damage(states,token,true,8,parameters)==StormboltPowerCellPromotion::unavailable);
    }
    {
        StormboltPowerCellStore<1> states;
        const auto token=states.initialize(0x3000);
        Bytes<0xA98> resource{};
        put(resource.data(),0xA80,1.25f);put(resource.data(),0xA84,0.75f);put(resource.data(),0xA88,0.5f);
        put(resource.data(),0xA8C,std::uint32_t{4});put(resource.data(),0xA90,std::uint32_t{2});
        constexpr float expected[]{5.0f,5.0f,3.0f,3.0f,2.0f,3.0f};
        for(std::uint32_t count=0;count<6;++count) {
            assert(states.transact(token,[&](auto& state) noexcept {state.count=count;return true;}));
            float accumulator=4.0f;
            assert(apply_stormbolt_power_cell_multiplier(states,token,true,resource.data(),accumulator)==StormboltPowerCellMultiplierResult::applied);
            assert(accumulator==expected[count]);
            StormboltPowerCellState snapshot{};assert(states.snapshot(token,snapshot) && snapshot.count==count);
        }
        float accumulator=7.0f;
        assert(apply_stormbolt_power_cell_multiplier(states,{},false,nullptr,accumulator)==StormboltPowerCellMultiplierResult::ineligible && accumulator==7.0f);
        assert(apply_stormbolt_power_cell_multiplier(states,token,true,nullptr,accumulator)==StormboltPowerCellMultiplierResult::unavailable && accumulator==7.0f);
        assert(states.retire(token));
        assert(apply_stormbolt_power_cell_multiplier(states,token,true,resource.data(),accumulator)==StormboltPowerCellMultiplierResult::unavailable && accumulator==7.0f);
    }
    {
        StormboltPowerCellStore<1> states;
        StormboltPowerCellLifecycle<1> lifecycle(states);
        Bytes<8> component{};
        unsigned calls=0;
        const auto valid=[](void*) {return true;};
        auto created=lifecycle.construct(component.data(),[&]() -> void* {
            ++calls;
            const auto inner=lifecycle.reset(component.data(),[&] {++calls;},valid);
            assert(!inner);
            return component.data();
        },valid);
        assert(created.native_result==component.data() && created.token && calls==2);
        assert(states.transact(created.token,[](auto& state) noexcept {state.count=4;return true;}));
        const auto reset=lifecycle.reset(component.data(),[&] {
            ++calls;StormboltPowerCellState state{};assert(!states.snapshot(created.token,state));
        },valid);
        StormboltPowerCellState state{};
        assert(reset && reset.generation!=created.token.generation);
        assert(states.snapshot(reset,state) && state.count==0 && state.timer==-1.0f);
        const auto native_result=lifecycle.destroy(component.data(),[&] {
            ++calls;assert(!states.snapshot(reset,state));return 17;
        });
        assert(native_result==17 && calls==4);
        const auto rejected=lifecycle.construct(component.data(),[&]() -> void* {++calls;return component.data();},[](void*) {return false;});
        assert(rejected.native_result==component.data() && !rejected.token && calls==5);
        const auto reused=lifecycle.construct(component.data(),[&]() -> void* {++calls;return component.data();},valid);
        assert(reused.token && calls==6);
        g_scenario=99;
        try {
            lifecycle.reset(component.data(),[&] {power_cell_reset_fault(calls);},valid);
            assert(false);
        } catch(const std::runtime_error&) {}
        assert(calls==7 && !states.snapshot(reused.token,state));
        g_scenario=0;
    }
    {
        StormboltPowerCellStore<1> states;
        const auto first=states.initialize(0x1000);
        assert(first);
        StormboltPowerCellState state{};
        assert(states.snapshot(first,state) && state.count==0 && state.timer==-1.0f);
        assert(states.transact(first,[](auto& next) noexcept {next.count=4;next.timer=150.0f;return true;}));
        assert(!states.initialize(0x2000)); // Do not evict a live character.
        const auto replacement=states.initialize(0x1000);
        assert(replacement && replacement.generation!=first.generation);
        state={9,9.0f};
        assert(!states.snapshot(first,state) && state.count==9 && state.timer==9.0f);
        assert(states.snapshot(replacement,state) && state.count==0 && state.timer==-1.0f);
        bool called=false;
        assert(!states.transact(first,[&](auto&) noexcept {called=true;return true;}) && !called);
        assert(!states.transact(replacement,[](auto& next) noexcept {next.count=7;return false;}));
        assert(states.snapshot(replacement,state) && state.count==0);
        assert(states.retire(replacement));
        const auto reused=states.initialize(0x2000);
        assert(reused && states.snapshot(reused,state) && state.count==0 && state.timer==-1.0f);
    }
    {
        constexpr std::uint32_t expected[]{0,0,1,1,2,1};
        for(std::uint32_t count=0;count<6;++count)
            assert(select_stormbolt_power_cell_multiplier(count,4,2)==expected[count]);
        assert(select_stormbolt_power_cell_multiplier(0,0,0)==2);
        assert(select_stormbolt_power_cell_multiplier(1,0,0)==1);
        assert(select_stormbolt_power_cell_multiplier(0,1,2)==0);
        assert(select_stormbolt_power_cell_multiplier(1,1,2)==2);
        assert(select_stormbolt_power_cell_multiplier(0xFFFFFFFF,1,2)==1);
        assert(select_stormbolt_power_cell_multiplier(0xFFFFFFFE,0xFFFFFFFF,1)==1);
        assert(select_stormbolt_power_cell_multiplier(0xFFFFFFFF,0xFFFFFFFF,1)==2);
    }
    {
        std::uint32_t count=4;
        float timer=150.0f;
        assert(advance_stormbolt_power_cell_timer(count,timer,false,1.0f)==StormboltPowerCellTimerResult::inactive);
        assert(count==4 && timer==150.0f);
        assert(advance_stormbolt_power_cell_timer(count,timer,true,149.0f)==StormboltPowerCellTimerResult::advanced);
        assert(count==4 && timer==1.0f);
        assert(advance_stormbolt_power_cell_timer(count,timer,true,1.0f)==StormboltPowerCellTimerResult::expired);
        assert(count==0 && timer==-1.0f);
        assert(advance_stormbolt_power_cell_timer(count,timer,true,1.0f)==StormboltPowerCellTimerResult::inactive);
        count=3; timer=0.0f;
        assert(advance_stormbolt_power_cell_timer(count,timer,true,0.0f)==StormboltPowerCellTimerResult::expired);
        assert(count==0 && timer==-1.0f);
        std::uint32_t nan_bits=0x7FC00001;
        std::memcpy(&timer,&nan_bits,4); count=4;
        assert(advance_stormbolt_power_cell_timer(count,timer,true,1.0f)==StormboltPowerCellTimerResult::inactive);
        std::uint32_t actual_bits{};std::memcpy(&actual_bits,&timer,4);
        assert(count==4 && actual_bits==nan_bits);
        const float nan_step=timer;timer=1.0f;
        assert(advance_stormbolt_power_cell_timer(count,timer,true,nan_step)==StormboltPowerCellTimerResult::advanced);
        assert(count==4 && timer!=timer);
        timer=1.0f;
        assert(advance_stormbolt_power_cell_timer(count,timer,true,-1.0f)==StormboltPowerCellTimerResult::advanced);
        assert(count==4 && timer==2.0f);
    }
    {
        Bytes<0xA98> resource{};
        put(resource.data(),0xA80,1.25f);
        put(resource.data(),0xA84,0.75f);
        put(resource.data(),0xA88,0.5f);
        put(resource.data(),0xA8C,std::uint32_t{4});
        put(resource.data(),0xA90,std::uint32_t{2});
        put(resource.data(),0xA94,150.0f);
        StormboltPowerCellParameters result{10,3,30.0f};
        assert(read_stormbolt_power_cell_parameters(resource.data(),result));
        assert(result.threshold==4 && result.related_count==2 && result.duration==150.0f);
        assert((result.multipliers==std::array<float,3>{1.25f,0.75f,0.5f}));
        assert(!read_stormbolt_power_cell_parameters(nullptr,result));
        assert(result.threshold==4 && result.related_count==2 && result.duration==150.0f);
        put(resource.data(),0xA8C,std::uint32_t{0xFFFFFFFF});
        put(resource.data(),0xA90,std::uint32_t{0});
        put(resource.data(),0xA94,std::uint32_t{0x7FC00001});
        assert(read_stormbolt_power_cell_parameters(resource.data(),result));
        std::uint32_t duration_bits{};
        std::memcpy(&duration_bits,&result.duration,4);
        assert(result.threshold==0xFFFFFFFF && result.related_count==0 && duration_bits==0x7FC00001);
    }
    {
        const auto query=+[](void* actor,std::uint32_t skill,std::uint32_t value,std::int32_t fallback)->std::int32_t {
            assert(actor==g_actor.data() && skill==0xD1 && value==0 && fallback==0);
            ++g_queries;
            return static_cast<std::int32_t>(g_scenario)-1;
        };
        g_queries=0;
        for(g_scenario=0;g_scenario<3;++g_scenario) {
            const auto result=stormbolt_power_cell_query(g_actor.data(),query);
            assert(result.available && result.positive==(g_scenario==2));
        }
        const auto absent=stormbolt_power_cell_query(nullptr,query);
        const auto missing=stormbolt_power_cell_query(g_actor.data(),nullptr);
        assert(absent.available && !absent.positive && !missing.available && !missing.positive && g_queries==3);
        g_scenario=0;
    }
    {
        const auto request=+[](void* p,std::uint32_t slot,std::uint32_t group,std::uint32_t record,bool post)->void* {
            assert(p==g_actor.data() && slot==1 && group==0x7D0 && record==1 && post);
            ++g_effects;
            if(g_scenario==2) throw std::runtime_error("effect");
            return g_scenario==1 ? nullptr : g_event.data();
        };
        g_effects=0;
        for(g_scenario=0;g_scenario<2;++g_scenario) {
            const auto result=stormbolt_self_damage_effect(g_actor.data(),request);
            assert(result.invoked && result.native_result==(g_scenario==1 ? nullptr : g_event.data()));
        }
        assert(!stormbolt_self_damage_effect(nullptr,request).invoked);
        assert(!stormbolt_self_damage_effect(g_actor.data(),nullptr).invoked);
        assert(g_effects==2);
        g_scenario=2;bool threw=false;
        try {stormbolt_self_damage_effect(g_actor.data(),request);} catch(const std::runtime_error&) {threw=true;}
        assert(threw && g_effects==3);g_scenario=0;
    }
    {
        for(bool present : {false,true}) for(bool flag : {false,true})
        for(std::int32_t index : {-1,0,1}) for(std::int16_t value : {std::int16_t{-1},std::int16_t{0},std::int16_t{1}})
        for(std::uint32_t stage : {0u,7u,8u,9u,0xffffffffu})
        for(std::uint32_t state : {0u,5u,6u,0xffffffffu}) {
            const auto branch=stormbolt_self_damage_branch(stage,present,flag,index,value,state,5);
            const bool enabled=present && flag && index && value==1 && state!=5;
            assert(branch.conditional==enabled);
            assert(branch.raise_state_and_notify==(enabled && stage>=8 && state==0));
        }
    }
    {
        using Result=StormboltSelfDamageDispatch;
        const auto adjust=+[](void* p,float amount,float multiplier,bool minimum,bool force) {
            assert(p==g_actor.data() && multiplier==1.0f && minimum && !force);
            assert(amount==0.25f || amount==75.0f); ++g_fires;
        };
        g_fires=0;
        for(float amount : {0.25f,75.0f})
            assert(stormbolt_adjust_self_damage(g_actor.data(),amount,adjust)==Result::invoked);
        assert(g_fires==2);
        assert(stormbolt_adjust_self_damage(nullptr,1.0f,adjust)==Result::unavailable);
        assert(stormbolt_adjust_self_damage(g_actor.data(),1.0f,nullptr)==Result::unavailable);
        for(std::uint32_t bits : {0u,0x80000000u,0xBF800000u,0x7FC01234u}) {
            float amount{};std::memcpy(&amount,&bits,4);
            assert(stormbolt_adjust_self_damage(g_actor.data(),amount,adjust)==Result::skipped);
        }
        assert(g_fires==2);
    }
    {
        StormboltSelfDamageAmounts amounts{};
        for(unsigned i=0;i<9;++i) {amounts.normal[i]=float(i+1);amounts.conditional[i]=float(20+i);}
        for(unsigned stage=0;stage<11;++stage) for(bool conditional : {false,true}) {
            const auto result=stormbolt_self_damage_amount(stage,conditional,amounts,150.0f);
            const auto clamped=stage<9 ? stage : 8;
            const float expected=conditional ? (clamped==8 ? 75.0f : float(20+clamped)) : float(clamped+1);
            assert(result.value==expected && result.call_adjustment);
        }
        for(std::uint32_t bits : {0u,0x80000000u,0xBF800000u,0x7FC01234u,0x7F800000u}) {
            float value{};std::memcpy(&value,&bits,4);
            amounts.normal[2]=value;
            const auto direct=stormbolt_self_damage_amount(2,false,amounts,150.0f);
            std::uint32_t returned{};std::memcpy(&returned,&direct.value,4);
            assert(returned==bits && direct.call_adjustment==(bits==0x7F800000u));
            const auto half=stormbolt_self_damage_amount(8,true,amounts,value);
            assert(half.call_adjustment==(bits==0x7F800000u));
        }
    }
    for(unsigned scenario=0;scenario<12;++scenario) {
        setup(3,false,scenario);g_fires=0;g_queries=0;g_effects=0;
        std::array<std::uintptr_t,80> table{};
        auto original=+[](void* p){
            ++g_fires;
            if(g_scenario==4) put(p,0x9FC,std::uint32_t{1});
            if(g_scenario==5) put(p,0x9F8,std::uint32_t{0});
            if(g_scenario==6) put(p,0xC,std::uint8_t{0});
            if(g_scenario==7) put(p,0,std::uintptr_t{0});
        };
        table[0x228/8]=reinterpret_cast<std::uintptr_t>(original);
        put(g_weapon.data(),0,reinterpret_cast<std::uintptr_t>(table.data()));
        if(scenario==1) put(g_weapon.data(),0x9FC,std::uint32_t{1});
        if(scenario==2) assert(g_store.retire(g_token));
        void* manager=g_weapon.data();StormboltStageUpdateApi api{};
        api.gate={&manager,+[](void*,std::uint32_t)->std::uint8_t {++g_effects;return 0;}};
        StormboltWeaponUpdateHook<1> hook(g_store,api,0xFFFF8214,stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa);
        StormboltWeaponUpdateBindings bindings{reinterpret_cast<std::uintptr_t>(table.data()),original,
            +[](void* p){
                assert(g_fires==1);++g_queries;
                if(g_scenario==8) put(p,0x9FC,std::uint32_t{1});
                if(g_scenario==9) put(p,0x9F8,std::uint32_t{0});
                if(g_scenario==10) put(p,0xC,std::uint8_t{0});
                if(g_scenario==11) put(p,0,std::uintptr_t{0});
                return g_scenario!=3;
            }};
        assert(hook.install(bindings));
        reinterpret_cast<StormboltWeaponUpdate>(table[0x228/8])(g_weapon.data());
        assert(g_fires==1 && g_queries==unsigned(scenario==0 || scenario==3 || scenario>=8) && g_effects==unsigned(scenario==0));
        assert(hook.restore() && table[0x228/8]==reinterpret_cast<std::uintptr_t>(original));
    }
    for(unsigned scenario=0;scenario<5;++scenario) {
        setup(3,false,scenario);g_queries=0;g_fires=0;
        if(scenario==3) assert(g_store.retire(g_token));
        put(g_weapon.data(),0x2350,std::uint8_t{1});put(g_weapon.data(),0x27DC,std::uint32_t{123});
        put(g_other.data(),0xC,std::uint8_t{2});
        std::array<std::uint32_t,41> ammo{};ammo[7]=123;void* manager=g_weapon.data();
        StormboltStageUpdateApi api{};api.ammo_items=ammo.data();
        api.gate={&manager,+[](void*,std::uint32_t)->std::uint8_t {assert(g_fires==1);++g_queries;return 0;}};
        api.routes.owner={&manager,+[](void*)->void*{return g_other.data();}};
        auto original=+[](void* p) {
            assert(p==g_weapon.data());++g_fires;
            put(p,0x9B0,g_other.data()); // Must use the post-original owner.
            if(g_scenario==1) assert(g_store.retire(g_token));
            if(g_scenario==2 || g_scenario==3) g_token=g_store.initialize(reinterpret_cast<std::uintptr_t>(p));
            if(g_scenario==4) throw std::runtime_error("original update");
        };
        bool threw=false;
        try {
            const auto result=stormbolt_update_with_original(g_store,g_weapon.data(),original,api,0xFFFF8214,
                stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa);
            const auto expected=scenario==0 ? StormboltWeaponUpdateResult::extended :
                scenario==3 ? StormboltWeaponUpdateResult::original_only : StormboltWeaponUpdateResult::invalidated;
            assert(result==expected);
        } catch(const std::runtime_error&) {threw=true;}
        assert(threw==(scenario==4) && g_fires==1 && g_queries==unsigned(scenario==0));
        if(scenario==0) assert(get<std::uint32_t>(g_weapon.data(),0x27C8)==7);
    }
    // Part removal must select the existing inactive continuation after the
    // native update refreshes counts, even while authority remains enabled.
    for(bool local:{false,true}) for(unsigned selected=0;selected<43;++selected) {
        setup(3,local);g_queries=0;g_fires=0;g_scenario=local?1:0;
        put(g_weapon.data(),0x2350,std::uint8_t{1});
        put(g_weapon.data(),0x2388,std::uint32_t{1});
        put(g_weapon.data(),0x27C8,std::uint32_t{40});
        std::array<std::uint32_t,41> ammo{};
        for(unsigned n=0;n<ammo.size();++n) ammo[n]=1000+n;
        const auto item=selected<41?ammo[selected]:selected==41?0u:99999u;
        put(g_weapon.data(),0x27DC,item);put(g_actor.data(),0xC,std::uint8_t{2});
        const auto actor_before=g_actor;
        auto expected_weapon=g_weapon;
        put(expected_weapon.data(),0x2388,std::uint32_t{0});
        put(expected_weapon.data(),0x2350,std::uint8_t{0});
        const auto expected_ammo=!local?0u:selected<41?selected:selected==41?0xFFu:0u;
        put(expected_weapon.data(),0x27C8,expected_ammo);
        void* manager=g_weapon.data();StormboltStageUpdateApi api{};api.ammo_items=ammo.data();
        api.gate={&manager,+[](void*,std::uint32_t)->std::uint8_t {
            assert(g_fires==1 && get<std::uint32_t>(g_weapon.data(),0x2388)==0);
            ++g_queries;return 1;
        }};
        api.descriptor=+[](void*,std::uint32_t)->std::uint64_t {assert(false);return 0;};
        api.routes.owner={&manager,+[](void*)->void* {
            assert(!get<std::uint8_t>(g_weapon.data(),0x2350));++g_queries;
            return g_scenario?g_actor.data():g_other.data();
        }};
        const auto original=+[](void* p) {++g_fires;put(p,0x2388,std::uint32_t{0});};
        assert(stormbolt_update_with_original(g_store,g_weapon.data(),original,api,0xFFFF8214,
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa)==StormboltWeaponUpdateResult::extended);
        assert(g_fires==1 && g_queries==2 && g_weapon==expected_weapon && g_actor==actor_before);
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.counter==3 && state.action_timer==123 && state.decay_timer==456);
    }
    for(unsigned route=0;route<6;++route) {
        setup(route==2 ? 9 : 3,route==3,route);g_effects=0;
        Bytes<0x12620> actor{};Bytes<0x40> input{};Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(actor.data(),0xC,std::uint8_t{2});put(actor.data(),0x8938,std::uint32_t(route==3));
        put(actor.data(),0x76A8,std::uint8_t{1});put(actor.data(),0x6274,std::uint32_t{3});
        put(actor.data(),0x12608,input.data());put(actor.data(),0x7810,g_parameter.data());
        put(actor.data(),0x8808,component.data());put(component.data(),8,bank.data());
        put(g_weapon.data(),0x9B0,actor.data());put(g_weapon.data(),0x68,0.25f);
        put(g_weapon.data(),0x2388,std::uint32_t{1});put(g_weapon.data(),0x2350,std::uint8_t(route!=1));
        assert(g_store.transact(g_token,[route](auto& s) noexcept {s.input_effect_active=std::uint8_t(route==4);return true;}));
        void* manager=g_weapon.data();StormboltStageUpdateApi api{};
        api.gate={&manager,+[](void*,std::uint32_t)->std::uint8_t{return g_scenario!=0;}};
        api.descriptor=+[](void*,std::uint32_t)->std::uint64_t{return 0;};
        api.routes.owner={&manager,+[](void*)->void*{return g_other.data();}};
        api.routes.input={+[](void*,std::uint32_t){return false;}};
        api.routes.cleanup={+[](const void*){return false;},+[](void*){assert(false);},+[](void*,void*)->void*{assert(false);return nullptr;}};
        api.routes.attachment=+[](void*)->std::uint32_t{return 0xC8;};
        api.routes.effect={+[](const void*){return false;},+[](void*,std::uint32_t g,std::uint32_t r,std::uint32_t)->void* {
            assert(g==0x7DC && r==2);++g_effects;return nullptr;
        },+[](void*,void*)->void*{++g_effects;return nullptr;}};
        assert(stormbolt_run_stage_update(g_store,g_token,actor.data(),api,0xFFFF8214,
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.counter==(route==2 ? 9u : 3u));
        assert(state.decay_timer==((route==1 || route==3 || route==5) ? 455.75f : 456.0f));
        assert(state.action_timer==(route==3 ? 13.0f : 123.0f));
        assert(g_effects==(route==4 ? 2u : 0u));
        if(route==0) assert(!get<std::uint8_t>(g_weapon.data(),0x2350) && !get<std::uint32_t>(g_weapon.data(),0x27C8));
    }
    for(bool mode:{false,true}) for(bool alive:{false,true}) for(bool local:{false,true})
      for(std::uint32_t item:{0u,123u,999u}) {
        setup(3,local);g_queries=0;g_scenario=local ? 1 : 0;
        put(g_weapon.data(),0x2350,std::uint8_t(mode));put(g_actor.data(),0xC,std::uint8_t(alive ? 2 : 0));
        put(g_weapon.data(),0x27DC,item);put(g_weapon.data(),0x27C8,std::uint32_t{37});
        std::array<std::uint32_t,41> ammo{};ammo[7]=123;void* manager=g_weapon.data();
        StormboltDecayOwnerApi api{&manager,+[](void*)->void* {
            ++g_queries;assert(!get<std::uint8_t>(g_weapon.data(),0x2350));
            return g_scenario ? g_actor.data() : g_other.data();
        }};
        assert(stormbolt_run_inactive_update(g_store,g_token,g_actor.data(),api,ammo.data(),{},
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        const auto expected=!mode ? 37u : alive && local ? item==0 ? 0xFFu : item==123 ? 7u : 0u : 0u;
        assert(get<std::uint32_t>(g_weapon.data(),0x27C8)==expected && !get<std::uint8_t>(g_weapon.data(),0x2350));
        assert(g_queries==unsigned(mode && alive));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.counter==3 && state.action_timer==123 && state.decay_timer==456);
    }
    for(bool local:{false,true}) for(bool mode:{false,true}) for(bool enabled:{false,true})
      for(bool cleanup:{false,true}) for(unsigned stage:{3u,9u}) for(bool active:{false,true}) {
        setup(stage,local);g_queries=0;g_scenario=cleanup ? 1 : 0;
        put(g_weapon.data(),0x2350,std::uint8_t(mode));put(g_actor.data(),0x76A8,std::uint8_t(enabled));
        put(g_actor.data(),0x6274,std::uint32_t{3});
        assert(g_store.transact(g_token,[active](auto& s) noexcept {s.input_effect_active=std::uint8_t(active);return true;}));
        auto descriptor=+[](void* p,std::uint32_t channel)->std::uint64_t {
            assert(p==g_actor.data() && channel==0);++g_queries;
            StormboltStageState state{};assert(g_store.snapshot(g_token,state));
            if(get<std::uint32_t>(p,0x8938)&1) assert(!state.input_effect_active);
            return g_scenario ? 5 : 0;
        };
        StormboltStageSelection selected{};
        assert(stormbolt_read_stage_selection(g_store,g_token,g_actor.data(),descriptor,selected));
        const auto expected=!mode || !enabled || cleanup ? StormboltStageRoute::cleanup :
            stage>=9 ? StormboltStageRoute::high_stage : local ? StormboltStageRoute::local_input :
            active ? StormboltStageRoute::remote_effect : StormboltStageRoute::remote_decay;
        assert(selected.route==expected && selected.local==local);
        assert(g_queries==unsigned(mode && enabled) && selected.group==(mode && enabled ? 3u : 0u));
    }
    for(unsigned route=0;route<3;++route) {
        setup(3,true);g_flags=0;g_effects=0;
        Bytes<0x12620> actor{};Bytes<0x40> input{};Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(actor.data(),0xC,std::uint8_t(route ? 2 : 0));put(actor.data(),0x12608,input.data());
        put(actor.data(),0x6278,std::uint32_t(route==1 ? 30 : 29));
        put(actor.data(),0x8808,component.data());put(actor.data(),0x7810,g_parameter.data());
        put(component.data(),8,bank.data());put(g_weapon.data(),0x9B0,actor.data());
        put(g_weapon.data(),0x68,0.25f);put(g_weapon.data(),0x2358,1.0f);
        assert(g_store.transact(g_token,[](auto& s) noexcept {s.input_timer=0.1f;s.input_latch=1;return true;}));
        StormboltLocalUpdateApi api{};
        api.input={+[](void*,std::uint32_t){return true;}};
        api.trigger={+[](void*,std::uint32_t){return true;},+[](void*,std::uint32_t){return true;}};
        api.attachment=+[](void*)->std::uint32_t{return 0xC8;};
        api.effect={+[](const void*){return false;},+[](void*,std::uint32_t g,std::uint32_t r,std::uint32_t)->void* {
            assert(g==0x7DC && r==2);++g_effects;return nullptr;
        },+[](void*,void*)->void*{++g_effects;return nullptr;}};
        api.cleanup={+[](const void*){return false;},+[](void*){assert(false);},+[](void*,void*)->void*{assert(false);return nullptr;}};
        api.actor_flag=+[](void*,std::uint32_t id){assert(id==0x16C);++g_flags;};
        assert(stormbolt_run_local_update(g_store,g_token,actor.data(),3,api,0xFFFF8214,
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.action_timer==(route==1 ? 13.0f : route==2 ? 122.75f : 123.0f));
        assert(state.decay_timer==(route==1 ? 455.75f : 456.0f));
        assert(state.input_timer<0 && state.input_latch==1 && state.counter==3);
        assert(g_flags==unsigned(route==2) && g_effects==(route==2 ? 2u : 0u));
    }
    for(bool alive:{false,true}) for(bool pressed:{false,true}) for(unsigned group:{2u,3u})
      for(unsigned index:{29u,30u,64u}) {
        setup(3,true);g_queries=0;
        Bytes<0x12620> actor{};Bytes<0x40> input{};
        put(actor.data(),0xC,std::uint8_t(alive ? 2 : 0));put(actor.data(),0x12608,input.data());
        put(actor.data(),0x6278,index);put(input.data(),0x10,pressed);put(g_weapon.data(),0x68,0.25f);
        assert(g_store.transact(g_token,[](auto& s) noexcept {s.input_latch=0x80;s.input_timer=0.1f;return true;}));
        StormboltLocalSelection selected{};
        StormboltLocalInputApi api{+[](void* p,std::uint32_t id) {
            assert(id==0x78);++g_queries;
            StormboltStageState s{};assert(g_store.snapshot(g_token,s));
            assert(s.input_latch==1 && s.input_timer<0);return get<bool>(p,0);
        }};
        assert(stormbolt_select_local_input(g_store,g_token,actor.data(),group,api,selected));
        const auto expected=!alive ? StormboltLocalRoute::fade_only :
            pressed && group==3 && index!=30 ? StormboltLocalRoute::trigger : StormboltLocalRoute::decay;
        assert(selected.route==expected && selected.held==1 && g_queries==unsigned(alive));
    }
    for(unsigned scenario=0;scenario<16;++scenario) {
        setup(3,true,scenario);g_effects=0;g_flags=0;
        Bytes<0x12620> actor{};Bytes<0x40> input{};Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(actor.data(),0x12608,input.data());put(actor.data(),0x8808,component.data());
        put(component.data(),8,bank.data());put(g_weapon.data(),0x9B0,actor.data());
        put(g_weapon.data(),0x2358,(scenario&8) ? 1.0f : 0.0f);put(g_weapon.data(),0x68,0.25f);
        assert(g_store.transact(g_token,[](auto& s) noexcept {s.input_timer=9;s.input_latch=0;return true;}));
        StormboltInputTriggerApi input_api{+[](void*,std::uint32_t){return (g_scenario&4)!=0;},
            +[](void*,std::uint32_t){return true;}};
        StormboltDecayEffectApi effect{+[](const void*) {
            assert(g_store.transact(g_token,[](auto& s) noexcept {s.input_latch=1;return true;}));
            return (g_scenario&1)!=0;
        },+[](void*,std::uint32_t group,std::uint32_t record,std::uint32_t attachment)->void* {
            assert(group==0x7DC && record==2 && attachment==0xC8);++g_effects;return nullptr;
        },+[](void*,void* p)->void*{assert(!p);++g_effects;return nullptr;}};
        auto flag=+[](void* p,std::uint32_t id) {
            assert(p==get<void*>(g_weapon.data(),0x9B0) && id==0x16C);++g_flags;
        };
        assert(stormbolt_run_local_trigger_route(g_store,g_token,actor.data(),(scenario&2)?1:0,
            input_api,+[](void*)->std::uint32_t{return 0xC8;},effect,flag,0xFFFF8214,{}, {},
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        const bool triggered=(scenario&4) && (scenario&8);
        const bool reset=triggered && (scenario&1) && !(scenario&2);
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.input_timer==(reset ? 0.1f : 9.0f));
        assert(state.input_effect_active==unsigned(triggered));
        assert(state.action_timer==(triggered ? 122.75f : 123.0f));
        assert(state.counter==3 && state.decay_timer==456 && g_flags==1);
        assert(g_effects==(triggered && !(scenario&1) ? 2u : 0u));
    }
    for(bool primary:{false,true}) for(bool alternate_gate:{false,true}) {
        Bytes<0x12620> actor{};Bytes<0x40> input{};
        put(actor.data(),0x12608,input.data());put(input.data(),0x10,primary);
        put(input.data(),0x11,alternate_gate);g_queries=0;
        StormboltInputTriggerApi api{+[](void* p,std::uint32_t index) {
            assert(index==0x27 && g_queries++==0);return get<bool>(p,0);
        },+[](void* p,std::uint32_t index) {
            assert(index==0x27 && g_queries++==1);return get<bool>(p,1);
        }};
        bool permitted=true;
        assert(stormbolt_read_input_trigger(actor.data(),api,permitted));
        assert(permitted==(primary && alternate_gate) && g_queries==(primary ? 2u : 1u));
        permitted=true;assert(!stormbolt_read_input_trigger(nullptr,api,permitted) && permitted);
        put(actor.data(),0x12608,static_cast<void*>(nullptr));
        assert(!stormbolt_read_input_trigger(actor.data(),api,permitted) && permitted);
    }
    for(std::uint32_t bits:{0u,0x80000000u,0x3F800000u,0x7FC01234u}) {
        put(g_weapon.data(),0x2358,bits);
        put(g_weapon.data(),0x2394,std::uint32_t{0xDEADBEEF});
        float resource=17;
        assert(!stormbolt_read_input_resource(nullptr,resource) && resource==17);
        assert(stormbolt_read_input_resource(g_weapon.data(),resource));
        std::uint32_t actual{};std::memcpy(&actual,&resource,4);assert(actual==bits);
        assert(get<std::uint32_t>(g_weapon.data(),0x2358)==bits);
    }
    for(bool primary:{false,true}) for(bool alternate:{false,true}) for(bool required:{false,true})
      for(float resource:{-1.0f,0.0f,1.0f,std::numeric_limits<float>::quiet_NaN()})
      for(float timer:{-1.0f,0.0f,1.0f,std::numeric_limits<float>::quiet_NaN()}) {
        StormboltStageState state{};state.action_timer=timer;state.counter=4;
        state.input_latch=0x80;state.input_effect_active=0;state.decay_timer=19;
        const bool accepted=primary && (!required || alternate) && resource>0;
        assert(stormbolt_advance_input_trigger(state,{primary,alternate,required,resource},0.25f)==accepted);
        if(std::isnan(timer)) assert(std::isnan(state.action_timer));
        else assert(state.action_timer==(accepted && timer>=0 ? timer-0.25f : timer));
        assert(state.counter==4 && state.input_latch==0x80 && !state.input_effect_active && state.decay_timer==19);
    }
    for(unsigned stage:{0u,1u,4u,7u,9u}) for(float timer:{1.0f,2.0f}) {
        setup(stage,false);g_effects=0;g_queries=0;
        Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(component.data(),8,bank.data());put(g_actor.data(),0x8808,component.data());
        put(g_weapon.data(),0x68,1.0f);
        assert(g_store.transact(g_token,[timer](auto& s) noexcept {
            s.decay_timer=timer;s.effect_latch=1;return true;
        }));
        StormboltCleanupBankApi cleanup{+[](const void*){++g_queries;return false;},
            +[](void*){assert(false);},+[](void*,void*)->void*{assert(false);return nullptr;}};
        StormboltDecayEffectApi effect{+[](const void*){return false;},
            +[](void*,std::uint32_t group,std::uint32_t record,std::uint32_t attachment)->void* {
                StormboltStageState state{};assert(g_store.snapshot(g_token,state));
                assert(group==0x7DB && attachment==0xFFFFFFFF);
                assert(record==(state.counter>=6 ? 2u : state.counter>=3 ? 1u : 0u));
                ++g_effects;return nullptr;
            },+[](void*,void* value)->void*{assert(!value);++g_effects;return nullptr;}};
        assert(stormbolt_run_cleanup_route(g_store,g_token,g_actor.data(),cleanup,effect,{}, {},
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        const bool down=stage && timer==1;
        assert(g_queries==(down ? 3u : 2u));
        assert(g_effects==(down && stage>1 ? 2u : 0u));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.counter==(down ? stage-1 : stage) && !state.effect_latch);
        assert(state.action_timer==123 && state.shot_snapshot==77);
    }
    for(unsigned scenario=0;scenario<3;++scenario) {
        setup(3,false,scenario);g_effects=0;
        Bytes<0x20> component{};Bytes<0x1B60> bank{};put(component.data(),8,bank.data());
        auto selector=+[](void* weapon)->std::uint32_t {
            assert(weapon==g_weapon.data());
            assert(g_store.transact(g_token,[](auto& s) noexcept {s.counter=7;return true;}));
            if(g_scenario==1) assert(g_store.retire(g_token));
            return 0xC9;
        };
        StormboltDecayEffectApi api{+[](const void*){return false;},
            +[](void*,std::uint32_t group,std::uint32_t record,std::uint32_t attachment)->void* {
                assert(group==0x7DC && record==2 && attachment==0xC9);++g_effects;return nullptr;
            },+[](void*,void* value)->void* {
                assert(!value);++g_effects;
                if(g_scenario==2) assert(g_store.retire(g_token));
                return nullptr;
            }};
        assert(stormbolt_run_remote_effect(g_store,g_token,component.data(),selector,api,{},
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa)==(scenario==0));
        assert(g_effects==(scenario==1 ? 0u : 2u));
        if(scenario==0) {
            StormboltStageState state{};assert(g_store.snapshot(g_token,state));
            assert(state.counter==7 && state.action_timer==123 && state.decay_timer==456);
        }
    }
    for(unsigned stage=0;stage<10;++stage) for(unsigned attachment=0xC8;attachment<=0xCA;++attachment)
      for(bool active:{false,true}) {
        Bytes<0x20> component{};Bytes<0x1B60> bank{};put(component.data(),8,bank.data());
        g_scenario=active ? 1 : 0;g_fires=stage;g_flags=attachment;g_effects=0;
        StormboltDecayEffectApi api{+[](const void*){return g_scenario!=0;},
          +[](void*,std::uint32_t group,std::uint32_t record,std::uint32_t parameter)->void*{
            static constexpr unsigned expected[]={0,1,1,2,2,2,3,3,3,0};
            assert(group==0x7DC && record==expected[g_fires] && parameter==g_flags);++g_effects;return nullptr;
          },+[](void*,void* value)->void*{assert(!value && g_effects==1);++g_effects;return nullptr;}};
        assert(stormbolt_update_remote_effect(component.data(),stage,attachment,api));
        assert(g_effects==(active ? 0u : 2u));
    }

    for(std::uint32_t stage:{9u,0xFFFFFFFFu}) for(bool initial_local:{false,true})
      for(bool cleared:{false,true}) for(bool later_local:{false,true}) {
        setup(stage,initial_local,cleared ? 1 : 0);g_effects=0;
        Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(component.data(),8,bank.data());put(g_other.data(),0x8938,std::uint32_t(later_local));
        assert(g_store.transact(g_token,[](auto& s) noexcept {s.input_effect_active=0x80;return true;}));
        StormboltCleanupBankApi cleanup{+[](const void*){return g_scenario!=0;},+[](void*){assert(false);},
            +[](void*,void* value)->void*{assert(!value);++g_effects;put(g_weapon.data(),0x9B0,g_other.data());return nullptr;}};
        assert(stormbolt_run_high_stage(g_store,g_token,component.data(),initial_local,cleanup,{},
            stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa)); // Model gate inactive.
        StormboltStageState current{};assert(g_store.snapshot(g_token,current));
        assert(current.input_effect_active==((cleared ? later_local : initial_local) ? 0x80 : 0));
        assert(current.counter==stage && current.action_timer==123 && current.decay_timer==456);
        assert(g_effects==unsigned(cleared));
    }

    for(unsigned scenario=0;scenario<8;++scenario) {
        setup(6,true);put(g_weapon.data(),0x68,0.5f);g_effects=0;
        void* materials[2]={scenario==4 ? nullptr : g_actor.data(),g_other.data()};
        put(g_weapon.data(),0x9F8,scenario==0 ? 12u : 13u);
        put(g_weapon.data(),0x1D00,scenario==1 ? 23u : 24u);
        put(g_weapon.data(),0x2B8,scenario==2 ? 0u : 2u);
        put(g_weapon.data(),0x2B0,scenario==3 ? static_cast<void*>(nullptr) : materials);
        StormboltMaterialColorApi api{+[](void* material,std::uint32_t hash,const float* color){
            assert(material==g_actor.data() && hash==0x361F6075u);++g_effects;
            StormboltStageState s{};assert(g_store.snapshot(g_token,s));
            assert(s.attachment_blink_phase==1 && s.attachment_blink_timer==0.1f);
            assert(std::fabs(color[0]-68.0f)<0.0001f && color[2]==100 && color[3]==1);
        }};
        if(scenario==5)api.write_vector=nullptr;
        if(scenario==6)assert(g_store.retire(g_token));
        const bool ok=stormbolt_run_attachment_tail(g_store,g_token,stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa,api);
        assert(ok==(scenario!=3 && scenario!=5 && scenario!=6));
        assert(g_effects==(scenario==7 ? 1u : 0u));
        if(scenario<6) {
            StormboltStageState s{};assert(g_store.snapshot(g_token,s));
            assert(s.attachment_blink_phase==0 && s.attachment_blink_timer==-1);
        }
    }

    for(unsigned scenario=0;scenario<7;++scenario) {
        setup(6,true);void* first=scenario==5 ? nullptr : g_actor.data();
        put(g_weapon.data(),0x9F8,scenario==1 ? 12u : 13u);
        put(g_weapon.data(),0x1D00,scenario==2 ? 23u : 24u);
        put(g_weapon.data(),0x2B8,scenario==3 ? 0u : 1u);
        put(g_weapon.data(),0x2B0,scenario==4 ? static_cast<void*>(nullptr) : &first);
        void* output=g_other.data();
        const auto result=stormbolt_select_attachment_material(scenario==0 ? nullptr : g_weapon.data(),output);
        const auto expected=scenario==0 || scenario==4 ? StormboltAttachmentGate::unavailable : scenario==6 ? StormboltAttachmentGate::ready : StormboltAttachmentGate::inactive;
        assert(result==expected && output==(scenario==6 ? g_actor.data() : g_other.data()));
    }

    for(std::uint32_t stage:{5u,6u}) for(std::uint8_t phase:{std::uint8_t{0},std::uint8_t{1}})
      for(float timer:{0.25f,0.5f}) for(bool retire:{false,true}) {
        setup(stage,true,retire ? 9 : 0);put(g_weapon.data(),0x68,0.5f);g_effects=0;
        assert(g_store.transact(g_token,[&](auto& s) noexcept {s.attachment_blink_timer=timer;s.attachment_blink_phase=phase;return true;}));
        StormboltMaterialColorApi api{+[](void* material,std::uint32_t hash,const float* color){
            assert(material==g_other.data() && hash==0x361F6075u);++g_effects;
            StormboltStageState state{};assert(g_store.snapshot(g_token,state));
            const bool high=state.counter>=6;
            const float multiplier=high ? (state.attachment_blink_phase ? 100.0f : 1.0f) : 20.0f;
            assert(color[0]==(high ? 0.0f : multiplier) && color[1]==(high ? multiplier : 0.0f));
            assert(color[2]==0 && color[3]==0.25f);
            assert(state.action_timer==123 && state.decay_timer==456);
            if(g_scenario==9)assert(g_store.retire(g_token));
        }};
        assert(stormbolt_update_attachment_color(g_store,g_token,g_other.data(),{0,0.5f,1,0.25f},{2,0.5f,1,0.25f},api)==!retire);
        assert(g_effects==1);
        if(!retire) {
            StormboltStageState state{};assert(g_store.snapshot(g_token,state));
            assert(state.attachment_blink_timer==(stage<6 ? -1.0f : timer<0.5f ? 0.1f : 0.0f));
            assert(state.attachment_blink_phase==(stage<6 ? 0 : timer<0.5f ? phase^1 : phase));
        }
    }

    const std::array<std::array<float,3>,7> primary_colors{{{1,0,0},{1,1,0},{0,1,0},{0,1,1},{0,0,1},{1,0,1},{1,0,0}}};
    for(unsigned h=0;h<7;++h) {
        const auto c=stormbolt_attachment_rgba({float(h),0.5f,1.0f,0.25f},20.0f);
        for(unsigned i=0;i<3;++i)assert(c[i]==primary_colors[h][i]*20);
        assert(c[3]==0.25f);
    }
    const auto gray=stormbolt_attachment_rgba({NAN,0.25f,0.0f,-0.0f},4);
    assert(gray[0]==1 && gray[1]==1 && gray[2]==1 && std::signbit(gray[3]));
    const auto low_color=stormbolt_attachment_rgba(stormbolt_attachment_low_hlsa,20);
    const auto high_color=stormbolt_attachment_rgba(stormbolt_attachment_high_hlsa,100);
    assert(std::fabs(low_color[0])<0.0001f && std::fabs(low_color[1]-10.54f)<0.0001f && low_color[2]==20);
    assert(std::fabs(high_color[0]-68.0f)<0.0001f && std::fabs(high_color[1]-20.0f)<0.0001f && high_color[2]==100);

    // Exact zero does not toggle. Negative crossing toggles once, without catch-up.
    for(std::uint32_t stage:{5u,6u,9u}) for(std::uint8_t phase:{std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}})
      for(float timer:{-1.0f,0.0f,0.25f,0.5f,NAN}) {
        StormboltStageState s{};s.counter=stage;s.attachment_blink_phase=phase;s.attachment_blink_timer=timer;
        s.input_effect_active=0x80;s.effect_alpha=123;
        const float multiplier=advance_stormbolt_attachment_blink(s,0.25f);
        if(stage<6) {
            assert(s.attachment_blink_phase==0 && s.attachment_blink_timer==-1 && multiplier==20);
        } else {
            const bool toggles=timer<0.25f;
            const auto expected_phase=toggles ? phase^1 : phase;
            assert(s.attachment_blink_phase==expected_phase);
            if(std::isnan(timer)) assert(std::isnan(s.attachment_blink_timer));
            else assert(s.attachment_blink_timer==(toggles ? 0.1f : timer-0.25f));
            assert(multiplier==(expected_phase ? 100.0f : 1.0f));
        }
        assert(s.counter==stage && s.input_effect_active==0x80 && s.effect_alpha==123);
    }

    // Expected routes from PS control-flow destinations. Cleanup must win even
    // at >=9, and the local clear occurs before every route (including cleanup).
    for(bool local:{false,true}) for(bool mode_on:{false,true}) for(bool enabled:{false,true})
      for(bool cleanup_action:{false,true}) for(std::uint32_t stage:{0u,8u,9u,0xFFFFFFFFu})
        for(std::uint8_t active:{std::uint8_t{0},std::uint8_t{0x80}}) {
        StormboltStageState s{};s.counter=stage;s.input_effect_active=active;
        s.input_latch=0x80;s.input_timer=-2;s.action_timer=13;s.decay_timer=17;s.effect_latch=1;
        const auto route=stormbolt_select_stage_route(s,local,mode_on,enabled,cleanup_action ? 5 : 0);
        if(!mode_on || !enabled || cleanup_action) assert(route==StormboltStageRoute::cleanup);
        else if(stage>=9) assert(route==StormboltStageRoute::high_stage);
        else if(local) assert(route==StormboltStageRoute::local_input);
        else if(active) assert(route==StormboltStageRoute::remote_effect);
        else assert(route==StormboltStageRoute::remote_decay);
        assert(s.input_effect_active==(local ? 0 : active));
        assert(s.counter==stage && s.input_latch==0x80 && s.input_timer==-2);
        assert(s.action_timer==13 && s.decay_timer==17 && s.effect_latch==1);
    }

    for(std::uint8_t latch:{std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}})
      for(float timer:{-1.0f,0.0f,0.05f,0.2f,std::numeric_limits<float>::quiet_NaN()}) {
        StormboltStageState s{};s.input_latch=latch;s.input_timer=timer;
        const auto held=advance_stormbolt_input_window(s,0.1f);
        const bool advances=latch && timer>0;
        assert(held==(advances ? 1 : latch) && s.input_latch==held);
        if(std::isnan(timer)) assert(std::isnan(s.input_timer));
        else assert(s.input_timer==(advances ? timer-0.1f : timer));
        assert(s.input_effect_active==0);
    }
    for(std::uint8_t held:{std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}}) {
        StormboltStageState s{};s.input_latch=0x80;s.input_timer=-2;
        activate_stormbolt_input_window(s,held);
        assert(s.input_effect_active==1 && s.input_latch==(held ? 0x80 : 1));
        assert(s.input_timer==(held ? -2.0f : 0.1f));
    }
    for(bool local:{false,true}) for(bool associated_local:{false,true})
      for(std::uint32_t stage:{4u,7u}) for(float timer:{1.0f,2.0f}) {
        setup(stage,local);Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(component.data(),8,bank.data());put(g_actor.data(),0x8808,component.data());
        put(g_weapon.data(),0x998,associated_local ? g_actor.data() : g_other.data());put(g_weapon.data(),0x68,1.0f);
        for(unsigned row=0;row<9;++row) put(g_parameter.data(),0x160C+40*row,20.0f+row);
        assert(g_store.transact(g_token,[timer](auto& s) noexcept {s.decay_timer=timer;s.effect_latch=1;s.effect_alpha=100;return true;}));
        void* manager=g_other.data();
        StormboltCleanupBankApi cleanup{+[](const void*){return false;},+[](void*){assert(false);},
            +[](void*,void*)->void*{assert(false);return nullptr;}};
        StormboltDecayEffectApi effect{+[](const void*){return false;},
            +[](void*,std::uint32_t,std::uint32_t,std::uint32_t)->void*{return nullptr;},
            +[](void*,void*)->void*{return nullptr;}};
        StormboltDecayOwnerApi owner{&manager,+[](void*)->void*{return g_actor.data();}};
        StormboltEffectColorApi color{+[](const void*){return false;},+[](void*,const std::uint32_t*){assert(false);}};
        void* first_material=g_other.data();g_effects=0;
        put(g_weapon.data(),0x1D00,24u);put(g_weapon.data(),0x2B8,1u);put(g_weapon.data(),0x2B0,&first_material);
        StormboltMaterialColorApi material{+[](void* p,std::uint32_t hash,const float* rgba){
            assert(p==g_other.data() && hash==0x361F6075u);++g_effects;
            StormboltStageState current{};assert(g_store.snapshot(g_token,current));
            assert(current.attachment_blink_phase==(current.counter>=6 ? 1 : 0));
            assert(rgba[2]==(current.counter>=6 ? 100.0f : 20.0f) && rgba[3]==1);
        }};
        assert(stormbolt_run_decay_with_attachment(g_store,g_token,component.data(),local,false,ps_stormbolt_color_default,
            cleanup,effect,owner,color,material,stormbolt_attachment_low_hlsa,stormbolt_attachment_high_hlsa));
        assert(g_effects==1);
        const bool down=timer==1.0f;const auto next=down ? stage-1 : stage;
        const bool cleared=down && associated_local && next<=5;
        StormboltStageState s{};assert(g_store.snapshot(g_token,s));
        assert(s.counter==next && s.shot_snapshot==77 && s.effect_latch==unsigned(!cleared));
        assert(s.decay_timer==(down ? 20.0f+next : 1.0f));
        assert(s.action_timer==(local ? 10.0f+next : 123.0f));
        assert(s.effect_alpha==(local && !cleared ? 0.0f : 100.0f));
    }
    for(bool enabled:{false,true}) for(bool rising:{false,true}) for(bool active:{false,true}) {
        setup(3,true);Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(g_weapon.data(),0x68,0.25f);put(g_actor.data(),0x8808,component.data());
        put(component.data(),8,bank.data());put(bank.data(),0x1AA0,g_other.data());
        put(bank.data(),0x1AA8,std::uint8_t(active));
        assert(g_store.transact(g_token,[enabled](auto& s) noexcept {s.effect_alpha=100;s.effect_latch=std::uint8_t(enabled);return true;}));
        g_queries=g_effects=0;g_scenario=rising ? 227u : 36u;
        StormboltEffectColorApi api{+[](const void* b){
            StormboltStageState s{};assert(g_store.snapshot(g_token,s));
            assert(s.effect_alpha==(g_scenario==227 ? 227.5f : 36.25f));++g_queries;
            return get<std::uint8_t>(b,8)!=0;
        },+[](void* effect,const std::uint32_t* color){
            assert(effect==g_other.data() && *color==((g_scenario<<24)|0xFF8214));++g_effects;
        }};
        assert(stormbolt_update_effect_fade(g_store,g_token,rising,ps_stormbolt_color_default,api));
        assert(g_queries==unsigned(enabled) && g_effects==unsigned(enabled && active));
        StormboltStageState after{};assert(g_store.snapshot(g_token,after));
        assert(after.effect_alpha==(enabled ? (rising ? 227.5f : 36.25f) : 100.0f));
        assert(after.counter==3 && after.shot_snapshot==77 && after.action_timer==123 && after.decay_timer==456);
    }
    for(bool bank_active:{false,true}) for(unsigned first_kind=0;first_kind<3;++first_kind) {
        Bytes<0x20> component{};Bytes<0x1B60> bank{};g_other.fill(std::byte{0});
        put(component.data(),8,bank.data());put(bank.data(),0x1AA8,std::uint8_t(bank_active));
        put(bank.data(),0x1AA0,first_kind ? g_other.data() : nullptr);
        put(g_other.data(),0xC,std::uint8_t(first_kind==2 ? 2 : 0));g_effects=0;
        StormboltEffectColorApi api{+[](const void* b){return get<std::uint8_t>(b,8)!=0;},
            +[](void* effect,const std::uint32_t* color){
                assert(effect==g_other.data() && *color==0x7FFF8214u);++g_effects;
            }};
        assert(stormbolt_write_effect_color(component.data(),127.9f,ps_stormbolt_color_default,api)==(!bank_active || first_kind==2));
        assert(g_effects==unsigned(bank_active && first_kind==2));
    }
    struct PackedCase {float alpha;std::uint32_t high;};
    const PackedCase packed_cases[]={{0,0},{0.9f,0},{1.9f,0x01000000u},
        {127.9f,0x7F000000u},{255,0xFF000000u},{256,0},{-1.9f,0xFF000000u},
        {std::numeric_limits<float>::quiet_NaN(),0},{std::numeric_limits<float>::infinity(),0},
        {-std::numeric_limits<float>::infinity(),0},{2147483648.0f,0}};
    const auto saved_mxcsr=_mm_getcsr();
    _mm_setcsr(saved_mxcsr|0x80u); // Mask invalid conversion as in ordinary game execution.
    for(const auto& row:packed_cases) for(std::uint32_t color:{0u,0x12345678u,0xFFFFFFFFu})
        assert(stormbolt_pack_effect_color(row.alpha,color)==(row.high|(color&0x00FFFFFFu)));
    _mm_setcsr(saved_mxcsr);
    assert(StormboltStageState{}.effect_alpha==255.0f);
    for(std::uint8_t latch : {std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}})
      for(std::uint32_t bits : {0u,0x42c80000u,0x7fc01234u}) {
        StormboltStageState state{3,77,123,456};state.effect_latch=latch;
        std::memcpy(&state.effect_alpha,&bits,4);
        activate_stormbolt_effect_latch(state);
        std::uint32_t actual{};std::memcpy(&actual,&state.effect_alpha,4);
        assert(actual==(latch ? bits : 0x437f0000u) && state.effect_latch==1);
        assert(state.counter==3 && state.shot_snapshot==77 && state.action_timer==123 && state.decay_timer==456);
    }
    setup(3,true);
    assert(g_store.transact(g_token,[](auto& state) noexcept {state.effect_alpha=5;state.effect_latch=1;return true;}));
    g_token=g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data()));
    StormboltStageState initialized{};assert(g_store.snapshot(g_token,initialized));
    assert(initialized.effect_alpha==255 && initialized.effect_latch==0);
    struct AlphaCase {float initial,step,expected;bool rising;};
    const AlphaCase alpha_cases[]={
        {0,0.5f,255,true},{0,0.25f,127.5f,true},{250,1,255,true},
        {-100,0,-100,true},{100,-1,-410,true},{255,0,255,true},
        {255,0.5f,127.5f,false},{255,1,0,false},{10,1,0,false},
        {300,0,300,false},{100,-1,355,false},{0,0,0,false}};
    for(const auto& row:alpha_cases) for(bool enabled:{false,true}) {
        StormboltStageState state{3,77,123,456};state.effect_alpha=row.initial;state.effect_latch=std::uint8_t(enabled);
        assert(advance_stormbolt_effect_alpha(state,row.step,row.rising)==enabled);
        assert(state.effect_alpha==(enabled ? row.expected : row.initial));
        assert(state.counter==3 && state.shot_snapshot==77 && state.action_timer==123 && state.decay_timer==456);
    }
    for(bool rising:{false,true}) {
        StormboltStageState state{};state.effect_latch=1;state.effect_alpha=std::numeric_limits<float>::quiet_NaN();
        assert(advance_stormbolt_effect_alpha(state,1,rising) && std::isnan(state.effect_alpha));
        state.effect_alpha=0;
        assert(advance_stormbolt_effect_alpha(state,std::numeric_limits<float>::quiet_NaN(),rising) && std::isnan(state.effect_alpha));
    }
    for(std::uint32_t stage : {0u,5u,6u}) for(unsigned kind=0;kind<4;++kind) {
        setup(stage,true);Bytes<0x20> component{};Bytes<0x1B60> bank{};
        put(component.data(),8,bank.data());put(g_actor.data(),0x8808,component.data());
        put(g_weapon.data(),0x998,kind==0 ? nullptr : static_cast<void*>(kind==3 ? g_other.data() : g_actor.data()));
        if(kind==1) put(g_actor.data(),0xC,std::uint8_t{0});
        assert(g_store.transact(g_token,[](auto& s) noexcept {s.effect_latch=7;return true;}));
        void* manager=g_other.data();g_queries=g_effects=0;
        StormboltDecayOwnerApi owner{&manager,+[](void* m)->void*{assert(m==g_other.data());++g_queries;return g_actor.data();}};
        StormboltCleanupBankApi clear{+[](const void*){++g_effects;return false;},
            +[](void*){assert(false);},+[](void*,void*)->void*{assert(false);return nullptr;}};
        assert(stormbolt_cleanup_decay_latch(g_store,g_token,owner,clear));
        const bool clears=kind==2 && stage<=5;
        assert(g_queries==unsigned(kind>=2) && g_effects==unsigned(clears));
        StormboltStageState s{};assert(g_store.snapshot(g_token,s));
        assert(s.effect_latch==(clears ? 0 : 7) && s.counter==stage && s.action_timer==123.0f && s.decay_timer==456.0f);
    }
    for(std::uint32_t stage=0;stage<=8;++stage)
      for(std::uint32_t bits : {0u,0x80000000u,0x3fc00000u,0x7fc01234u,0x7f800000u}) {
        setup(stage,true);
        put(g_parameter.data(),0x1608+40*stage,bits);
        assert(stormbolt_reload_local_decay_action_timer(g_store,g_token));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        std::uint32_t actual{};std::memcpy(&actual,&state.action_timer,4);
        assert(actual==bits && state.counter==stage && state.shot_snapshot==77 && state.decay_timer==456.0f);
    }
    for(unsigned fault=0;fault<4;++fault) {
        setup(fault==0 ? 9u : 3u,true);
        if(fault==1) put(g_weapon.data(),0x9B0,static_cast<void*>(nullptr));
        if(fault==2) put(g_actor.data(),0x7810,static_cast<void*>(nullptr));
        auto token=g_token;if(fault==3) ++token.generation;
        assert(!stormbolt_reload_local_decay_action_timer(g_store,token));
        StormboltStageState state{};assert(g_store.snapshot(g_token,state));
        assert(state.action_timer==123.0f && state.decay_timer==456.0f && state.shot_snapshot==77);
    }
    for(auto row : {std::pair{0u,0u},std::pair{1u,0u},std::pair{2u,0u},std::pair{3u,1u},
        std::pair{5u,1u},std::pair{6u,2u},std::pair{8u,2u},std::pair{9u,0u},std::pair{0xffffffffu,0u}})
      for(bool active : {false,true}) for(bool null_result : {false,true}) for(bool change_buffer : {false,true}) {
        const auto stage=row.first;
        Bytes<0x20> component{}; Bytes<0x1B60> bank{},replacement{};
        put(component.data(),8,bank.data()); put(bank.data(),0x1988,std::uint8_t(active));
        put(g_other.data(),0x40,null_result ? nullptr : static_cast<void*>(g_actor.data()));
        put(g_other.data(),0x48,change_buffer ? replacement.data() : bank.data());
        g_scenario=row.second;
        g_effects=g_fires=g_queries=0;
        StormboltDecayEffectApi api{
            +[](const void* b){++g_queries;return get<std::uint8_t>(b,8)!=0;},
            +[](void* c,std::uint32_t group,std::uint32_t record,std::uint32_t parameter)->void*{
                assert(group==0x7DB && record==g_scenario && parameter==0xFFFFFFFF);
                ++g_effects;put(c,8,get<void*>(g_other.data(),0x48));return get<void*>(g_other.data(),0x40);
            },+[](void* b,void* result)->void*{
                assert(b==static_cast<std::byte*>(get<void*>(g_other.data(),0x48))+0x1980);
                assert(result==get<void*>(g_other.data(),0x40));++g_fires;return b;
            }};
        assert(stormbolt_update_decay_effect(component.data(),stage,api));
        assert(g_queries==unsigned(stage!=0));
        assert(g_effects==unsigned(stage!=0 && !active) && g_fires==g_effects);
    }
    Bytes<0x12610> input_actor{};
    StormboltLocalInputApi input_api{+[](void* input,std::uint32_t index){
        assert(input==g_other.data()+0x10 && index==0x78);++g_queries;
        return get<std::uint8_t>(input,0)!=0;
    }};
    std::uint32_t flags_output=0x12345678;
    g_queries=0;
    assert(!stormbolt_read_local_input_flags(nullptr,input_api,flags_output));
    assert(!stormbolt_read_local_input_flags(input_actor.data(),{},flags_output));
    assert(!stormbolt_read_local_input_flags(input_actor.data(),input_api,flags_output));
    assert(flags_output==0x12345678 && g_queries==0);
    put(input_actor.data(),0x12608,g_other.data());
    for(std::uint8_t raw : {std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}}) {
        put(g_other.data(),0x10,raw);
        assert(stormbolt_read_local_input_flags(input_actor.data(),input_api,flags_output));
        assert(flags_output==(raw ? 0x01000000u : 0u));
    }
    assert(g_queries==3);
    for(std::uint32_t flags : {0u,0x01000000u,0xfeffffffu,0xffffffffu})
      for(std::uint32_t group : {0u,3u,4u,0xffffffffu})
        for(std::uint32_t index=0;index<66;++index) {
            // Independent interval transcription of the PS bitmap, including
            // the unsigned >63 branch before BT (no modulo-64 shift).
            const bool listed=(index>=30 && index<=32) ||
                (index>=43 && index<=51) || (index>=55 && index<=63);
            const bool decay=!(flags&0x01000000u) || group!=3 || listed;
            assert((stormbolt_local_input_path(flags,group,index)==
                StormboltLocalInputPath::decay_branch)==decay);
        }
    assert(stormbolt_local_input_path(0x01000000u,3,0xffffffffu)==
        StormboltLocalInputPath::additional_input_checks);
    for(std::uint64_t high : {0ull,0x40ull,0x100000000ull,0xffffffffffffffc0ull})
      for(std::uint64_t category=0;category<64;++category) {
        const auto index=category-5;
        const bool expected=index<=31 && ((0x80800481ull>>index)&1)!=0;
        assert(stormbolt_action_selects_cleanup(high|category)==expected);
      }
    void* authority=g_actor.data();
    for(std::uint8_t value : {std::uint8_t{0},std::uint8_t{1},std::uint8_t{0x80}})
      for(std::uint32_t count : {0u,1u,0xffffffffu}) {
        put(g_actor.data(),0,value); put(g_weapon.data(),0x2388,count); g_queries=0;
        StormboltUpdateGateApi gate{&authority,+[](void* manager,std::uint32_t index)->std::uint8_t {
            assert(manager==g_actor.data() && index==0); ++g_queries;
            return get<std::uint8_t>(manager,0);
        }};
        assert(stormbolt_update_common_gate(g_weapon.data(),gate)==(value && count ?
            StormboltUpdateGateResult::continue_checks : StormboltUpdateGateResult::inactive));
        assert(g_queries==1);
    }
    assert(stormbolt_update_common_gate(g_weapon.data(),{})==StormboltUpdateGateResult::unavailable);
    StormboltUpdateGateApi refresh{&authority,+[](void*,std::uint32_t)->std::uint8_t {
        put(g_weapon.data(),0x2388,0u); return 1;
    }};
    put(g_weapon.data(),0x2388,1u);
    assert(stormbolt_update_common_gate(g_weapon.data(),refresh)==StormboltUpdateGateResult::inactive);

    StormboltLastEntry<1> entry(g_store,{mode,angle,motion});
    for(auto stage : {0u,1u,8u,9u,0xffffffffu}) for(bool local : {false,true}) {
        setup(stage,local);
        assert(entry.enter(g_action.data()).status==StormboltSpecialActionStatus::processed);
        StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
        assert(state.counter==(local ? stage : 8u) && state.shot_snapshot==77);
        assert(state.action_timer==(stage<=8 ? 10.0f+stage : 123.0f) && state.decay_timer==0.5f);
        assert((g_calls==std::vector<unsigned>{1,2,3}));
        assert(action_field<std::uint8_t>(0x34)==0);
    }
    for(unsigned which=1;which<=6;++which) {
        setup(2,true,which);
        if(which==2) put(g_other.data(),0x76B0,static_cast<void*>(nullptr)); // Retain original g_weapon.
        if(which==3) put(g_other.data(),0x8938,std::uint8_t{0}); // Re-read after motion.
        const auto result=entry.enter(g_action.data());
        assert(result.status==(which<4 ? StormboltSpecialActionStatus::processed : StormboltSpecialActionStatus::invalidated));
        assert(action_field<std::uint8_t>(0x34)==(which<4 ? 0 : 1));
        assert(g_calls.size()==(which==4 ? 2u : 3u));
        if(which<4) {
            StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
            assert(state.counter==(which==3 ? 8u : 2u));
            assert(state.action_timer==(which==2 ? 22.0f : 12.0f));
        }
    }
    setup(0,true);
    StormboltLastEntry<1> missing(g_store,{mode,angle,nullptr});
    assert(missing.enter(g_action.data()).status!=StormboltSpecialActionStatus::processed && g_calls.empty());
    assert(action_field<std::uint32_t>(0x30)==9);
    StormboltLastUpdate<1> update(g_store,{update_event,update_difference,update_origin,
        update_fire,update_flag,update_effect,update_complete});
    for(unsigned phase : {0u,1u,2u}) for(bool shot : {false,true})
      for(bool effect : {false,true}) for(bool fired : {false,true}) for(bool completed : {false,true}) {
        setup(8,true);
        g_fires=g_effects=g_queries=g_flags=0; g_completed=completed; g_delta=0;
        put(g_action.data(),0x30,phase); put(g_action.data(),0x34,std::uint8_t(fired));
        const auto bits=0xA4u | (shot ? 1u : 0u) | (effect ? 0x40000000u : 0u);
        put(g_event.data(),0x28,bits);
        put(g_event.data(),0x190,0x40000001u); // Middle bank must never trigger Last.
        const auto result=update.update(g_action.data());
        const bool did_fire=phase==1 && shot;
        assert(get<std::uint32_t>(g_event.data(),0x190)==0x40000001u);
        assert(result.status==StormboltSpecialActionStatus::processed && result.animation_complete==completed);
        assert(g_fires==unsigned(did_fire) && g_effects==unsigned(effect) && g_queries==1);
        assert(g_flags==unsigned(!fired && !did_fire));
        assert(action_field<std::uint32_t>(0x30)==(phase==0 ? 1u : phase));
        assert(get<std::uint32_t>(g_event.data(),0x28)==(0xA4u | (shot && !did_fire ? 1u : 0u)));
        StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
        assert(state.counter==(did_fire ? 9u : 8u));
        assert(state.shot_snapshot==(did_fire ? 8u : 77u));
    }
    for(float delta : {30.0f,-30.0f,30.01f,-30.01f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        setup(0,true); g_delta=delta; g_fires=0;
        put(g_action.data(),0x30,0u); put(g_event.data(),0x28,1u);
        assert(update.update(g_action.data()).status==StormboltSpecialActionStatus::processed);
        assert(action_field<std::uint32_t>(0x30)==unsigned(std::fabs(delta)<=30));
        assert(g_fires==0 && get<std::uint32_t>(g_event.data(),0x28)==1);
    }
    setup(0xffffffff,true); g_fire_throw=true; g_fires=0;
    put(g_action.data(),0x30,1u); put(g_action.data(),0x34,std::uint8_t{0}); put(g_event.data(),0x28,1u);
    bool threw=false;
    try { update.update(g_action.data()); } catch(const std::runtime_error&) { threw=true; }
    assert(threw && g_fires==1 && action_field<std::uint8_t>(0x34)==0);
    StormboltStageState wrapped{}; assert(g_store.snapshot(g_token,wrapped));
    assert(wrapped.counter==0 && wrapped.shot_snapshot==0xffffffff && wrapped.action_timer==10.0f);
    g_fire_throw=false;
    assert(update.update(g_action.data()).status==StormboltSpecialActionStatus::processed && g_fires==1);
    setup(0,true); g_effect_ok=false; g_queries=0; put(g_event.data(),0x28,0x40000000u);
    assert(update.update(g_action.data()).status==StormboltSpecialActionStatus::invalidated);
    assert(g_queries==0 && get<std::uint32_t>(g_event.data(),0x28)==0);
    g_effect_ok=true;
    setup(2,true);
    std::array<std::uintptr_t,9> table{};
    const auto original_boundary=+[](void*){};
    const auto original_update=+[](void*){return false;};
    table[6]=table[8]=reinterpret_cast<std::uintptr_t>(original_boundary);
    table[7]=reinterpret_cast<std::uintptr_t>(original_update);
    const auto original_table=table;
    put(g_action.data(),0,reinterpret_cast<std::uintptr_t>(table.data()));
    put(g_actor.data(),0,std::uintptr_t{0x123400}); put(g_weapon.data(),0,std::uintptr_t{0x567800});
    Bytes<0x400> component{}; Bytes<0x1B60> bank{};
    put(component.data(),8,bank.data()); put(g_actor.data(),0x8808,component.data());
    void* slot=component.data();
    StormboltPlayerEffectApi effects{};
    effects.parameter_dti=component.data(); effects.provider_context=component.data();
    effects.provider_slot=&slot; effects.post_manager_slot=&slot;
    effects.construct_parameter=+[](void*,void* p)->void*{return p;};
    effects.actor_selector=+[](void*,void*){};
    effects.create=+[](void*,void*,void*,std::uint32_t,std::uint32_t,void*,void*,void*)->void*{assert(false);return nullptr;};
    effects.post_needed=+[](void*)->std::uint8_t{return 0;};
    effects.post=+[](void*,void*,const void*,std::uint32_t){};
    effects.bank_active=+[](const void*){++g_effects;return true;};
    effects.assign_bank=+[](void*,void*)->void*{assert(false);return nullptr;};
    effects.clear_bank70=+[](void*){};
    StormboltSpecialToNormalBindings binding{reinterpret_cast<std::uintptr_t>(table.data()),0x123400,0x567800,
        original_boundary,original_boundary,original_update,+[](void*,StormboltSpecialActionPhase){return true;}};
    const StormboltLastUpdateApi update_api{update_event,update_difference,update_origin,update_fire,update_flag,nullptr,update_complete};
    {
        auto incomplete=effects; incomplete.create=nullptr;
        StormboltLastSession<1> rejected(g_store,{mode,angle,motion},update_api,incomplete,binding);
        assert(!rejected.install() && rejected.restore() && table==original_table);
    }
    StormboltLastSession<1> session(g_store,{mode,angle,motion},update_api,effects,binding);
    assert(session.install());
    reinterpret_cast<StormboltActionBoundary>(table[6])(g_action.data());
    g_delta=0;g_completed=false;g_effects=g_fires=0;
    put(g_event.data(),0x28,0x40000001u);
    assert(!reinterpret_cast<StormboltActionUpdate>(table[7])(g_action.data()));
    assert(g_effects==1 && g_fires==0); // Owned effect context, no same-frame fire.
    g_completed=true;
    assert(reinterpret_cast<StormboltActionUpdate>(table[7])(g_action.data()) && g_fires==1);
    reinterpret_cast<StormboltActionBoundary>(table[8])(g_action.data());
    StormboltStageState reset_state{};assert(g_store.snapshot(g_token,reset_state));
    assert(reset_state.counter==0 && reset_state.shot_snapshot==2);
    assert(session.restore() && table==original_table);
    for(unsigned changed : {8u,7u,6u}) {
        assert(session.install());
        const auto owned=table;
        const auto foreign=reinterpret_cast<std::uintptr_t>(+[](void*){});
        table[changed]=foreign;
        assert(!session.restore() && table[changed]==foreign);
        for(unsigned retained=6;retained<changed;++retained) assert(table[retained]==owned[retained]);
        for(unsigned restored=changed+1;restored<=8;++restored) assert(table[restored]==original_table[restored]);
        table[changed]=owned[changed];
        assert(session.restore() && table==original_table);
    }
    for(std::size_t offset : {0x1980u,0x19E0u,0x1AA0u}) for(bool first_live : {false,true}) {
        Bytes<0x1B60> replacement{};
        put(component.data(),8,bank.data());
        put(bank.data(),offset,g_other.data()); put(g_other.data(),0xC,std::uint8_t(first_live ? 2 : 0));
        g_effects=g_fires=0;
        // Reuse actor storage as fixture context without global adapter state.
        put(g_other.data(),0x20,component.data()); put(g_other.data(),0x28,replacement.data());
        StormboltCleanupBankApi cleanup{+[](const void*){return true;},+[](void* object){
            ++g_fires; auto* c=get<void*>(object,0x20); auto* b=get<void*>(object,0x28); put(c,8,b);
        },+[](void* target,void* result)->void*{assert(!result);++g_effects;put(target,0,std::uint64_t{0x1234});return target;}};
        assert(stormbolt_cleanup_bank(component.data(),offset,cleanup));
        assert(g_fires==unsigned(first_live) && g_effects==1);
        assert(get<std::uint64_t>((first_live ? replacement.data() : bank.data()),offset)==0x1234);
    }
    for(bool first_active : {false,true}) {
        Bytes<0x400> second_component{}; Bytes<0x1B60> first_bank{},second_bank{};
        put(component.data(),8,first_bank.data()); put(second_component.data(),8,second_bank.data());
        put(g_actor.data(),0x8808,component.data()); put(g_other.data(),0x8808,second_component.data());
        put(g_weapon.data(),0x9B0,g_actor.data());
        put(first_bank.data(),0x19E0+8,std::uint8_t(first_active));
        put(first_bank.data(),0x1AA0+8,std::uint8_t{1}); put(second_bank.data(),0x1AA0+8,std::uint8_t{1});
        g_effects=0;
        StormboltCleanupBankApi cleanup{+[](const void* b){
            // Deliberately change owner even when first bank is inactive: the
            // native predicate is pure, but this fixture exposes extra rereads.
            put(g_weapon.data(),0x9B0,g_other.data());
            return get<std::uint8_t>(b,8)!=0;
        },+[](void*){assert(false);},+[](void* b,void* value)->void*{
            assert(!value);++g_effects;put(b,16,std::uint32_t{0x5678});return b;
        }};
        assert(stormbolt_cleanup_stage_effects(g_weapon.data(),g_actor.data(),cleanup));
        assert(g_effects==(first_active ? 2u : 1u));
        assert(get<std::uint32_t>(first_active ? second_bank.data() : first_bank.data(),0x1AA0+16)==0x5678);
        assert(get<std::uint32_t>(first_active ? first_bank.data() : second_bank.data(),0x1AA0+16)==0);
    }
    for(std::uint32_t stage : {0u,1u,8u,9u,0xffffffffu})
      for(float remaining : {2.0f,1.0f,0.0f,-10.0f,std::numeric_limits<float>::quiet_NaN()}) {
        setup(stage,true); put(g_actor.data(),0x8808,component.data()); put(component.data(),8,bank.data());
        put(g_weapon.data(),0x68,1.0f);
        for(unsigned row=0;row<9;++row) put(g_parameter.data(),0x160C+40*row,20.0f+row);
        assert(g_store.transact(g_token,[remaining](auto& state) noexcept {
            state.decay_timer=remaining;state.effect_latch=1;return true;
        }));
        StormboltCleanupBankApi no_effect{+[](const void*){return false;},+[](void*){assert(false);},
            +[](void*,void*)->void*{assert(false);return nullptr;}};
        assert(stormbolt_cleanup_and_decay(g_store,g_token,g_actor.data(),no_effect));
        StormboltStageState after{};assert(g_store.snapshot(g_token,after));
        const bool decreases=stage && remaining<=1.0f;
        const auto expected=decreases ? (std::min)(stage-1,8u) : stage;
        assert(after.counter==expected && after.shot_snapshot==77 && after.action_timer==123.0f && !after.effect_latch);
        if(decreases) assert(after.decay_timer==20.0f+expected);
        else if(std::isnan(remaining)) assert(std::isnan(after.decay_timer));
        else assert(after.decay_timer==(stage ? remaining-1.0f : remaining));
    }
    for(std::uint32_t stage : {0u,1u,4u,7u,9u})
      for(float timer : {2.0f,1.0f,-3.0f,std::numeric_limits<float>::quiet_NaN()}) {
        setup(stage,true);bank.fill(std::byte{0});put(component.data(),8,bank.data());
        put(g_actor.data(),0x8808,component.data());put(g_weapon.data(),0x68,1.0f);
        for(unsigned row=0;row<9;++row) put(g_parameter.data(),0x160C+40*row,30.0f+row);
        assert(g_store.transact(g_token,[timer](auto& s) noexcept {s.decay_timer=timer;s.effect_latch=7;return true;}));
        g_calls.clear();
        StormboltCleanupBankApi clear{+[](const void* b){
            g_calls.push_back(get<std::uint32_t>(b,16));return true;
        },+[](void*){assert(false);},+[](void* b,void* value)->void*{
            assert(!value);g_calls.push_back(3);return b;
        }};
        put(bank.data(),0x19E0+16,1u);put(bank.data(),0x1980+16,2u);
        StormboltDecayEffectApi effect{+[](const void*){g_calls.push_back(4);return false;},
            +[](void*,std::uint32_t group,std::uint32_t,std::uint32_t parameter)->void*{
                assert(group==0x7DB && parameter==0xFFFFFFFF);g_calls.push_back(5);return nullptr;
            },+[](void*,void* value)->void*{assert(!value);g_calls.push_back(6);return nullptr;}};
        const auto result=stormbolt_decay_prefix(g_store,g_token,component.data(),clear,effect);
        const bool down=stage && timer<=1.0f;
        const auto expected=down ? stage-1 : stage;
        assert(result==(down ? StormboltDecayPrefixResult::continue_after_effect : StormboltDecayPrefixResult::no_decrement));
        StormboltStageState s{};assert(g_store.snapshot(g_token,s));
        assert(s.counter==expected && s.effect_latch==7 && s.shot_snapshot==77 && s.action_timer==123.0f);
        if(down) assert(s.decay_timer==30.0f+expected);
        else if(std::isnan(timer)) assert(std::isnan(s.decay_timer));
        else assert(s.decay_timer==(stage ? timer-1 : timer));
        const std::vector<unsigned> expected_calls= !down ? std::vector<unsigned>{1,3} :
            expected ? std::vector<unsigned>{1,3,2,3,4,5,6} : std::vector<unsigned>{1,3,2,3};
        assert(g_calls==expected_calls);
    }
    std::puts("PASS: Last entry stage boundaries, remote ordering, actor/owner rereads; Last update 48-case matrix and event ordering; no game calls");
}
