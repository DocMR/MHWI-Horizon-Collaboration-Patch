#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_middle_fire_adapter.h"
#include "stormbolt_middle_prepare_adapter.h"
#include "stormbolt_middle_update.h"
#include "stormbolt_middle_entry.h"
#include "stormbolt_last_exit.h"
#include "stormbolt_middle_session.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes=std::array<std::byte,N>;
Bytes<0x38> g_action;
Bytes<0x9000> g_actor;
Bytes<0x9000> g_entry_actor2;
Bytes<0x2860> g_weapon;
Bytes<0x176C> g_parameter;
Bytes<0x200> g_event;
StormboltInstanceStore<1> g_store;
StormboltInstanceToken g_token;
unsigned g_fires=0, g_origins=0;
bool g_throw=false, g_retire=false;
bool g_native_fields=false;
unsigned g_boundary_fires=0;
float g_difference=0;
unsigned g_motions=0, g_rates=0;
std::vector<std::uint32_t> g_flags;
unsigned g_queries=0;
bool g_complete=false;
unsigned g_fallback_enter=0,g_fallback_update=0;
void original_enter(void*) { ++g_fallback_enter; }
bool original_update(void*) { ++g_fallback_update; return true; }
bool foreign_update(void*) { return false; }
void foreign_enter(void*) { }
bool fixture_ready(void*,StormboltSpecialActionPhase) { return true; }
bool g_need=false;
unsigned g_modes=0,g_parameters=0,g_reads=0,g_writes=0;
std::uint8_t g_network_stage=0;
unsigned g_entry_scenario=0;
void* g_expected_entry_actor=g_actor.data();
void replace_entry_actor(void* actor) {
    g_expected_entry_actor=actor;
    std::memcpy(g_action.data()+0x28,&actor,sizeof(actor));
}
void entry_mode(void* actor,std::uint32_t value) {
    assert(actor==g_actor.data() && value==1); ++g_modes;
    if(g_entry_scenario==1) replace_entry_actor(g_entry_actor2.data());
    if(g_entry_scenario==3) replace_entry_actor(nullptr);
}
bool needs_parameter(void* actor) { assert(actor==g_expected_entry_actor && g_modes==1); return g_need; }
bool entry_parameter(void* actor,float value,std::uint32_t mode) {
    assert(actor==g_expected_entry_actor && value==1.75f && mode==2); ++g_parameters;
    StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
    if(g_entry_scenario==2) replace_entry_actor(g_entry_actor2.data());
    if(g_entry_scenario==4) assert(g_store.retire(g_token));
    return true;
}
std::uint8_t read_stage(void* actor) { assert(actor==g_expected_entry_actor); ++g_reads; return g_network_stage; }
void write_stage(void* actor,std::uint8_t value) { assert(actor==g_expected_entry_actor); ++g_writes; g_network_stage=value; }
void flag(void* actor,std::uint32_t value) { assert(actor==g_actor.data()); g_flags.push_back(value); }
bool query(void* actor,std::uint32_t channel) {
    assert(actor==g_actor.data() && channel==0); ++g_queries; return g_complete;
}
template<class T> void put(void* p,std::size_t o,T v) { std::memcpy(static_cast<std::byte*>(p)+o,&v,sizeof(v)); }
template<class T> T get(void* p,std::size_t o) { if(g_native_fields && p==g_action.data() && (o==0x30 || o==0x34)) return StormboltActionFields<1>(g_store).template get<T>(p,o); T v{}; std::memcpy(&v,static_cast<std::byte*>(p)+o,sizeof(v)); return v; }
void* event(void* actor) { assert(actor==g_actor.data()); return g_event.data(); }
const void* origin(void* actor) {
    assert(actor==g_actor.data()); ++g_origins;
    if(g_retire) assert(g_store.retire(g_token));
    return g_actor.data()+0x7D40;
}
void fire(void* weapon,const void* point,bool a,std::uint32_t b,bool c) {
    assert(weapon==g_weapon.data() && point==g_actor.data()+0x7D40 && a && b==0xFFFFFFFF && c);
    StormboltStageState state{};
    assert(g_store.snapshot(g_token,state)); // Proves callbacks are outside the sidecar lock.
    assert(state.shot_snapshot==g_fires && state.counter==g_fires+1 && state.decay_timer==0.5f);
    assert(get<std::uint32_t>(g_event.data(),0x190)==0xA4);
    assert(get<std::uint8_t>(g_action.data(),0x34)==0);
    ++g_fires;
    if(g_throw) throw std::runtime_error("native firing fixture");
}
float difference(void* actor) { assert(actor==g_actor.data()); return g_difference; }
bool motion(void* actor,std::uint32_t packed,std::uint32_t id,std::uint32_t mode,
    std::uint32_t extra,float start,float rate) {
    const auto layer=g_motions++;
    assert(actor==g_actor.data() && layer<3 && packed==0x200+layer && id==0xC0F4-layer);
    assert(mode==(layer<2 ? 1u : 3u) && !extra && start==-1 && rate==2);
    return false;
}
void rate(void* actor,std::uint32_t layer,float value) {
    assert(actor==g_actor.data() && layer==1 && value==1.25f && g_motions==3);
    assert(get<std::uint32_t>(g_action.data(),0x30)==0); ++g_rates;
}
void setup() {
    g_action.fill(std::byte{0}); g_fires=g_origins=0; g_throw=g_retire=false;
    put(g_action.data(),0x28,g_actor.data()); put(g_action.data(),0x30,std::uint32_t{1});
    put(g_actor.data(),0xC,std::uint8_t{2}); put(g_actor.data(),0x76B0,g_weapon.data());
    put(g_actor.data(),0x7810,g_parameter.data()); put(g_weapon.data(),0xC,std::uint8_t{2});
    put(g_weapon.data(),0x9F8,std::uint32_t{13}); put(g_weapon.data(),0x9FC,std::uint32_t{239});
    put(g_weapon.data(),0x9B0,g_actor.data()); put(g_event.data(),0x190,std::uint32_t{0xA5});
    for(unsigned i=0;i<9;++i) put(g_parameter.data(),0x1608+i*40,10.0f+i);
    put(g_parameter.data(),0x1768,0.5f);
    g_token=g_store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data())); assert(g_token);
}
}
int main() {
    StormboltMiddleFireAdapter<1> adapter(g_store,{event,origin,fire});
    setup();
    for(unsigned shot=0;shot<10;++shot) {
        put(g_event.data(),0x190,std::uint32_t{0xA5}); put(g_action.data(),0x34,std::uint8_t{0});
        assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::returned);
        assert(get<std::uint8_t>(g_action.data(),0x34)==1);
        StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
        assert(state.action_timer==10.0f+(std::min)(shot+1,8u));
        assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::waiting);
        assert(g_fires==shot+1);
    }
    setup(); put(g_action.data(),0x30,std::uint32_t{0});
    assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::waiting && !g_fires && !g_origins);
    setup(); g_throw=true; bool threw=false;
    try { adapter.apply(g_action.data()); } catch(const std::runtime_error&) { threw=true; }
    assert(threw && g_fires==1 && get<std::uint8_t>(g_action.data(),0x34)==0);
    assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::waiting);
    setup(); g_retire=true;
    assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::invalidated && g_fires==0);
    assert(get<std::uint32_t>(g_event.data(),0x190)==0xA4);
    StormboltLastExit<1> last_exit(g_store,flag);
    for(bool fired : {false,true}) for(std::uint32_t bits : {0x3f800000u,0x7fc12345u,0x7f800000u}) {
        setup(); g_flags.clear();
        put(g_action.data(),0x34,std::uint8_t(fired));
        put(g_parameter.data(),0x1608,bits); put(g_parameter.data(),0x1768,bits);
        assert(g_store.transact(g_token,[](auto& state) noexcept {
            state={8,7,3.0f,4.0f}; return true;
        }));
        assert(last_exit.exit(g_action.data()).status==StormboltSpecialActionStatus::processed);
        StormboltStageState after{}; assert(g_store.snapshot(g_token,after));
        assert(after.counter==(fired ? 0u : 8u) && after.shot_snapshot==7);
        std::uint32_t action_bits{},decay_bits{};
        std::memcpy(&action_bits,&after.action_timer,4); std::memcpy(&decay_bits,&after.decay_timer,4);
        assert(action_bits==(fired ? bits : 0x40400000u));
        assert(decay_bits==(fired ? bits : 0x40800000u));
        assert(g_flags==std::vector<std::uint32_t>{0xDB});
        assert(get<std::uint8_t>(g_action.data(),0x34)==std::uint8_t(fired));
    }
    StormboltMiddlePrepareApi prepare_api{};
    prepare_api.animation.motion=motion; prepare_api.difference=difference; prepare_api.rate=rate;
    StormboltMiddlePrepareAdapter<1> prepare(g_store,prepare_api);
    for(float delta : {0.0f,30.0f,-30.0f,30.01f,-30.01f,
        std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        setup(); g_difference=delta; g_motions=g_rates=0;
        put(g_action.data(),0x30,std::uint32_t{0}); put(g_parameter.data(),0x840,1.25f);
        const bool allowed=std::fabs(delta)<=30;
        assert(prepare.apply(g_action.data())==(allowed ? StormboltMiddlePrepareResult::armed : StormboltMiddlePrepareResult::waiting));
        assert(g_motions==(allowed ? 3u : 0u) && g_rates==(allowed ? 1u : 0u));
        assert(g_fires==0 && get<std::uint32_t>(g_event.data(),0x190)==0xA5);
        assert(get<std::uint32_t>(g_action.data(),0x30)==(allowed ? 1u : 0u));
        if(allowed) assert(adapter.apply(g_action.data())==StormboltMiddleFireResult::returned);
    }
    StormboltSpecialToNormalApi outer{}; outer.actor_flag=flag; outer.query=query;
    StormboltMiddleUpdate<1> update(g_store,prepare_api,{event,origin,fire},outer);
    for (unsigned phase : {0u,1u,2u}) for (bool pending : {false,true})
        for (bool complete : {false,true}) for (bool fired : {false,true}) {
            setup(); g_flags.clear(); g_queries=g_motions=g_rates=0;
            g_difference=0; g_complete=complete;
            put(g_parameter.data(),0x840,1.25f);
            put(g_action.data(),0x30,phase);
            put(g_action.data(),0x34,std::uint8_t(fired));
            put(g_event.data(),0x190,std::uint32_t(pending ? 0xA5 : 0xA4));
            // The fire fixture checks the pre-return marker; exercise previously
            // fired updates without a second pending event in this matrix.
            if (phase==1 && pending && fired) continue;
            const auto result=update.update(g_action.data());
            assert(result.status==StormboltSpecialActionStatus::processed);
            assert(result.animation_complete==(phase==1 && complete));
            assert(g_queries==(phase==1 ? 1u : 0u));
            assert(g_fires==(phase==1 && pending ? 1u : 0u));
            assert(g_motions==(phase==0 ? 3u : 0u));
            std::vector<std::uint32_t> expected{0x5D,0x22,0x28,0x7E};
            if (!(phase==1 && complete) && !fired && !(phase==1 && pending)) expected.push_back(0x16D);
            assert(g_flags==expected);
        }
    StormboltMiddleEntry<1> entry(g_store,{entry_mode,needs_parameter,entry_parameter,read_stage,write_stage});
    for (bool need : {false,true}) for (bool gate : {false,true}) for (bool local : {false,true}) {
        setup(); g_need=need; g_modes=g_parameters=g_reads=g_writes=0; g_network_stage=201;
        put(g_parameter.data(),0x818,1.75f);
        put(g_actor.data(),0x60,std::uint8_t(gate)); put(g_actor.data(),0x8938,std::uint32_t(local));
        put(g_action.data(),0x34,std::uint8_t{1});
        assert(g_store.transact(g_token,[](auto& state) noexcept {
            state.counter=0x123; state.shot_snapshot=7; state.action_timer=9; state.decay_timer=11; return true;
        }));
        assert(entry.enter(g_action.data()).status==StormboltSpecialActionStatus::processed);
        assert(g_modes==1 && g_parameters==unsigned(need));
        assert(g_reads==unsigned(gate && !local) && g_writes==unsigned(gate && local));
        assert(g_network_stage==(gate && local ? 0x23 : 201));
        StormboltStageState state{}; assert(g_store.snapshot(g_token,state));
        assert(state.counter==(gate && !local ? 201u : 0x123u));
        assert(state.shot_snapshot==7 && state.action_timer==9 && state.decay_timer==11);
        assert(get<std::uint32_t>(g_action.data(),0x30)==0 && get<std::uint8_t>(g_action.data(),0x34)==0);
    }
    for(unsigned scenario=1; scenario<=4; ++scenario) {
        setup(); g_entry_scenario=scenario; g_expected_entry_actor=g_actor.data();
        g_modes=g_parameters=g_reads=g_writes=0; g_need=true;
        g_entry_actor2.fill(std::byte{0});
        put(g_entry_actor2.data(),0xC,std::uint8_t{2});
        put(g_entry_actor2.data(),0x60,std::uint8_t{1});
        // The new actor deliberately has no weapon/parameter. The PS entry
        // retains its initial weapon and reads that weapon owner's parameter.
        put(g_parameter.data(),0x818,1.75f);
        put(g_actor.data(),0x60,std::uint8_t{1});
        put(g_actor.data(),0x8938,std::uint32_t{0});
        put(g_action.data(),0x34,std::uint8_t{1});
        const auto result=entry.enter(g_action.data());
        assert(result.status==(scenario<=2 ? StormboltSpecialActionStatus::processed : StormboltSpecialActionStatus::invalidated));
        assert(g_parameters==(scenario==3 ? 0u : 1u));
        assert(g_writes==0 && g_reads==(scenario==3 ? 0u : 1u));
        assert(get<std::uint8_t>(g_action.data(),0x34)==(scenario<=2 ? 0u : 1u));
    }
    auto* table=static_cast<std::uintptr_t*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(table);
    table[6]=reinterpret_cast<std::uintptr_t>(&original_enter);
    table[7]=reinterpret_cast<std::uintptr_t>(&original_update);
    table[8]=0x12345678;
    setup(); g_entry_scenario=0; g_expected_entry_actor=g_actor.data();
    g_need=false; g_modes=g_parameters=g_reads=g_writes=0; g_network_stage=0;
    put(g_action.data(),0,reinterpret_cast<std::uintptr_t>(table));
    put(g_actor.data(),0,std::uintptr_t{0x11223344}); put(g_weapon.data(),0,std::uintptr_t{0x55667788});
    put(g_actor.data(),0x60,std::uint8_t{0});
    StormboltSpecialToNormalBindings binding{reinterpret_cast<std::uintptr_t>(table),0x11223344,0x55667788,
        original_enter,nullptr,original_update,fixture_ready};
    {
        auto unavailable=binding; unavailable.ready=nullptr;
        StormboltMiddleSession<1> missing(g_store,{entry_mode,needs_parameter,entry_parameter,read_stage,write_stage},
            prepare_api,{event,origin,fire},outer,unavailable);
        assert(!missing.install() && missing.restore());
    }
    StormboltMiddleSession<1> session(g_store,{entry_mode,needs_parameter,entry_parameter,read_stage,write_stage},
        prepare_api,{event,origin,fire},outer,binding);
    assert(session.install() && table[8]==0x12345678);
    // Primary and Middle share native bodies, but own distinct vtables/callback owners.
    auto* primary_table=table+32;
    primary_table[6]=reinterpret_cast<std::uintptr_t>(&original_enter);
    primary_table[7]=reinterpret_cast<std::uintptr_t>(&original_update);
    primary_table[8]=0x87654321;
    auto primary_binding=binding;primary_binding.action_vtable=reinterpret_cast<std::uintptr_t>(primary_table);
    StormboltMiddleSession<1,true> primary(g_store,{entry_mode,needs_parameter,entry_parameter,read_stage,write_stage},
        prepare_api,{event,origin,fire},outer,primary_binding);
    assert(primary.install());
    g_native_fields=true;
    const auto extension_canary=std::array<std::byte,8>{std::byte{0xA5},std::byte{0xA5},std::byte{0xA5},std::byte{0xA5},std::byte{0xA5},std::byte{0xA5},std::byte{0xA5},std::byte{0xA5}};
    std::memcpy(g_action.data()+0x30,extension_canary.data(),8);
    put(g_action.data(),0,reinterpret_cast<std::uintptr_t>(primary_table));
    reinterpret_cast<StormboltActionBoundary>(primary_table[6])(g_action.data());
    assert(g_modes==1 && !g_fallback_enter && primary_table[8]==0x87654321);
    assert(primary.restore() && primary_table[6]==reinterpret_cast<std::uintptr_t>(&original_enter));
    put(g_action.data(),0,reinterpret_cast<std::uintptr_t>(table));g_modes=0;

    reinterpret_cast<StormboltActionBoundary>(table[6])(g_action.data());
    assert(g_modes==1 && !g_fallback_enter);
    g_motions=g_rates=g_queries=0; g_difference=0;
    put(g_parameter.data(),0x840,1.25f);
    assert(!reinterpret_cast<StormboltActionUpdate>(table[7])(g_action.data()));
    assert(g_motions==3 && !g_queries && !g_fires);
    g_complete=true;
    assert(reinterpret_cast<StormboltActionUpdate>(table[7])(g_action.data()));
    assert(g_fires==1 && g_queries==1);
    assert(std::memcmp(g_action.data()+0x30,extension_canary.data(),8)==0);
    put(g_weapon.data(),0x9FC,std::uint32_t{237});
    assert(reinterpret_cast<StormboltActionUpdate>(table[7])(g_action.data()) && g_fallback_update==1);
    assert(session.restore());
    assert(table[6]==reinterpret_cast<std::uintptr_t>(&original_enter) && table[7]==reinterpret_cast<std::uintptr_t>(&original_update));
    for (const unsigned slot : {7u,6u}) {
        assert(session.install());
        const auto owned_enter=table[6],owned_update=table[7];
        const auto foreign=slot==7 ? reinterpret_cast<std::uintptr_t>(&foreign_update) : reinterpret_cast<std::uintptr_t>(&foreign_enter);
        table[slot]=foreign;
        assert(!session.restore() && table[slot]==foreign && table[8]==0x12345678);
        if(slot==7) {
            assert(table[6]==owned_enter);
            // Selected core remains alive after update-slot restoration failed.
            put(g_weapon.data(),0x9FC,std::uint32_t{239});
            g_modes=0;
            reinterpret_cast<StormboltActionBoundary>(table[6])(g_action.data());
            assert(g_modes==1);
        } else {
            assert(table[7]==reinterpret_cast<std::uintptr_t>(&original_update));
        }
        table[slot]=slot==7 ? owned_update : owned_enter;
        assert(session.restore());
        assert(table[6]==reinterpret_cast<std::uintptr_t>(&original_enter));
        assert(table[7]==reinterpret_cast<std::uintptr_t>(&original_update));
    }
    // Exact PC allocation followed immediately by an inaccessible page. Old
    // phase30/fired34 writes fault here instead of being hidden by PS-sized fixtures.
    auto* pages=static_cast<std::byte*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(pages);DWORD previous{};assert(VirtualProtect(pages+4096,4096,PAGE_NOACCESS,&previous));
    auto* native_action=pages+4096-0x30;
    std::memset(native_action,0,0x30);put(native_action,0x28,g_actor.data());
    std::array<std::byte,0x30> initial{};std::memcpy(initial.data(),native_action,0x30);
    StormboltMiddleEntryApi boundary_entry{+[](void*,std::uint32_t){},+[](void*){return false;},
        +[](void*,float,std::uint32_t){return true;},+[](void*)->std::uint8_t{return 0;},+[](void*,std::uint8_t){}};
    auto boundary_prepare=prepare_api;
    boundary_prepare.difference=+[](void*){return 0.0f;};
    boundary_prepare.rate=+[](void*,std::uint32_t,float){};
    boundary_prepare.animation.motion=+[](void*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,float,float){return true;};
    StormboltMiddleFireApi boundary_fire{event,+[](void* actor)->const void*{return actor;},
        +[](void*,const void*,bool,std::uint32_t,bool){++g_boundary_fires;}};
    auto boundary_outer=outer;boundary_outer.query=+[](void*,std::uint32_t){return false;};
    StormboltMiddleCore<1,true> primary_core(g_store,boundary_entry,boundary_prepare,boundary_fire,boundary_outer);
    StormboltMiddleCore<1> middle_core(g_store,boundary_entry,boundary_prepare,boundary_fire,boundary_outer);
    const auto run_boundary=[&](auto& core) {
        assert(g_store.transact(g_token,[](auto& state) noexcept {state.counter=0;return true;}));
        for(unsigned shot=0;shot<9;++shot) {
            assert(core.enter(native_action).status==StormboltSpecialActionStatus::processed);
            assert(core.update(native_action).status==StormboltSpecialActionStatus::processed);
            put(g_event.data(),0x190,1u);
            assert(core.update(native_action).status==StormboltSpecialActionStatus::processed);
            StormboltStageState state{};assert(g_store.snapshot(g_token,state));
            assert(state.action_owner==reinterpret_cast<std::uintptr_t>(native_action));
            assert(state.action_phase==1 && state.action_fired==1 && state.counter==shot+1);
            assert(std::memcmp(initial.data(),native_action,0x30)==0);
        }
    };
    run_boundary(primary_core);run_boundary(middle_core);assert(g_boundary_fires==18);
    assert(VirtualFree(pages,0,MEM_RELEASE));
    std::puts("PASS: primary/middle 18 shots with exact30-byte PC actions ending at PAGE_NOACCESS; extension state stays in instance store");
    assert(table[8]==0x12345678 && VirtualFree(table,0,MEM_RELEASE));
    std::puts("PASS: Middle scoped session, readiness/fallback, both restore conflicts and retry; no game activation");
}
