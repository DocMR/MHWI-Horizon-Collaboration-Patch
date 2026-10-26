#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_power_cell_session.h"
#include "stormbolt_damage_session_test_fixture.h"
#include "stormbolt_power_cell_growth_inputs.h"
#include "stormbolt_power_cell_growth_commit.h"
#include "stormbolt_power_cell_growth_effects.h"
#include "stormbolt_power_cell_growth_adapter.h"
#include "stormbolt_power_cell_post_effects.h"
#include "stormbolt_power_cell_remote_decision.h"
#include "stormbolt_power_cell_remote_commit.h"
#include "stormbolt_power_cell_remote_effects.h"
#include "stormbolt_power_cell_remote_adapter.h"
#include "stormbolt_power_cell_remote_cache.h"
#include "stormbolt_power_cell_remote_inputs.h"
#include "stormbolt_power_cell_remote_native_adapter.h"
#include "stormbolt_power_cell_sync_event.h"
#include "stormbolt_power_cell_sync_store.h"
#include "stormbolt_power_cell_growth_sync.h"
#include "stormbolt_power_cell_growth_complete_adapter.h"
#include "stormbolt_power_cell_sync_classification.h"
#include "stormbolt_power_cell_sync_snapshot.h"
#include "stormbolt_power_cell_sync_frames.h"
#include "stormbolt_sync_pack_dispatch.h"
#include "stormbolt_sync_complete_dispatch.h"
#include "stormbolt_power_cell_sync_codec.h"
#include "stormbolt_sync_producer_session.h"
#include "stormbolt_growth_dispatch.h"
#include "stormbolt_growth_session.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
extern "C" {
void stormbolt_timer_test_call(void*,void*);
bool stormbolt_timer_test_query(void*);
extern unsigned char stormbolt_timer_test_patch_site;
void* stormbolt_timer_test_seen=nullptr;
void* stormbolt_timer_test_query_owner=nullptr;
std::uintptr_t stormbolt_timer_test_query_value=1,stormbolt_timer_test_next_query_value=1;
unsigned stormbolt_timer_test_query_calls=0;
unsigned char stormbolt_timer_test_flag=0;
float stormbolt_timer_test_step=0.25f,stormbolt_timer_test_step_seen=0;

void stormbolt_loop_test_call(void*,void*);
void stormbolt_status_test_call(void*,void*);
extern unsigned char stormbolt_loop_test_patch_site,stormbolt_status_test_patch_site;
void* stormbolt_loop_test_actor=nullptr;
unsigned char stormbolt_loop_test_flag=0,stormbolt_loop_test_use_site=1;
void (*stormbolt_loop_test_entry)()=&stormbolt_loop_update_entry;
void* stormbolt_status_test_seen=nullptr;
std::uintptr_t stormbolt_status_test_manager=0xAAAA,stormbolt_status_test_next_manager=0;
unsigned char stormbolt_status_test_flag=0;
}
namespace {
std::array<std::byte,0x13D60> owner{};
std::array<std::byte,0x15A8> skills{};
std::array<std::byte,0xA10> status{};
std::array<std::byte,0x9D0> component{};
std::array<std::byte,0xA98> resource{};
std::array<std::byte,0x448> material{};
unsigned flags=0,parameters=0,prompts=0,plays=0;
int primary_first=0,primary_second=0;
unsigned post_requests=0;
StormboltPowerCellSyncStore<2>* complete_sync_store=nullptr;
unsigned complete_body_requests=0;
bool expected_sync_pending=false;
template<class T> void put(void* p,std::size_t o,const T& v) {std::memcpy(static_cast<std::byte*>(p)+o,&v,sizeof(v));}
__declspec(noinline) void status_reset(void* p,bool preserve_effects) {if(!preserve_effects) put(p,0x20,std::uint32_t{0});}
__declspec(noinline) void* status_construct(void* p,void* actor) {
    put(p,0,std::uintptr_t{2});put(p,8,actor);status_reset(p,false);return p;
}
__declspec(noinline) void status_destroy(void* p) {put(p,0,std::uintptr_t{0});put(p,8,static_cast<void*>(nullptr));}
__declspec(noinline) void* loop_construct(void* p) {put(p,0,std::uintptr_t{4});put(p,0x60,static_cast<void*>(owner.data()));return p;}
__declspec(noinline) void* loop_destroy(void* p,std::uint32_t f) {assert(f==5);put(p,0,std::uintptr_t{0});return p;}
__declspec(noinline) void* sync_construct(void* p) {put(p,0,std::uintptr_t{77});put(p,0xDB0,std::uintptr_t{0});return p;}
__declspec(noinline) void* sync_destroy(void* p,std::uint32_t f) {assert(f==0);put(p,0,std::uintptr_t{0});return p;}
StormboltNativeBindings bindings(std::uintptr_t* slot) {
    StormboltNativeBindings n{};
    damage_session_fixture::configure(n);
    n.power_cell_sync_construct=reinterpret_cast<std::uintptr_t>(&sync_construct);
    n.power_cell_sync_destroy=reinterpret_cast<std::uintptr_t>(&sync_destroy);
    n.power_cell_sync_vtable=77;
    n.power_cell={reinterpret_cast<std::uintptr_t>(&status_construct),reinterpret_cast<std::uintptr_t>(&status_reset),
        reinterpret_cast<std::uintptr_t>(&status_destroy),2,0,0};
    n.loop_construct=reinterpret_cast<std::uintptr_t>(&loop_construct);
    n.loop_destroy=reinterpret_cast<std::uintptr_t>(&loop_destroy);n.loop_vtable=4;
    n.loop_inputs={1,2,reinterpret_cast<std::uintptr_t>(slot),3};
    n.skill_value_query=+[](void*,std::uint32_t,std::uint32_t,std::int32_t) {return 1;};
    n.special_to_normal.action.actor_flag=+[](void* p,std::uint32_t f) {assert(p==owner.data() && f==0x98);++flags;};
    n.power_cell_material_at=+[](void*,std::uint32_t i)->void* {assert(i==0);return material.data();};
    n.power_cell_visual_convert=+[](const float* a,float* b)->float* {std::memcpy(b,a,16);return b;};
    n.power_cell_material_scalar=+[](void*,std::uint32_t,float) {++parameters;};
    n.material_color.write_vector=+[](void*,std::uint32_t,const float*) {++parameters;};
    n.power_cell_status_update_site=reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_patch_site);
    n.power_cell_status_manager_slot=reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_manager);
    n.loop_update_site=reinterpret_cast<std::uintptr_t>(&stormbolt_loop_test_patch_site);
    n.power_cell_timer_update_site=reinterpret_cast<std::uintptr_t>(&stormbolt_timer_test_patch_site);
    n.power_cell_timer_native_query=&stormbolt_timer_test_query;
    n.skill_prompt=+[](void* actor,std::uint32_t id,bool removal,std::int32_t value) {
        assert(actor==owner.data() && id==0x1D3 && !removal && value==-1);++prompts;
    };
    return n;
}
}
int main() {
    for (unsigned mask=0;mask<16;++mask) {
        const StormboltPowerCellRemoteFlags sent{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8)};
        for (std::uint32_t native_word:{0u,0x00FFFFFFu,0x00C00000u,0x70000000u}) {
            std::uint32_t encoded=0;
            assert(encode_stormbolt_power_cell_sync(native_word,sent,encoded));
            assert((encoded&~stormbolt_power_cell_sync_mask)==native_word);
            StormboltPowerCellRemoteCache wire{};wire.current[4]=encoded;wire.previous[4]=encoded;
            StormboltPowerCellRemoteFlags received{};bool previous=false;
            assert(decode_stormbolt_power_cell_sync(wire,received,previous));
            assert(received.empty==sent.empty && received.below==sent.below &&
                received.reached==sent.reached && received.effect==sent.effect && previous==sent.effect);
        }
    }
    std::uint32_t collided_output=0x12345678;
    assert(encode_stormbolt_power_cell_sync(0x01000000,{false,true,false,true},collided_output));
    assert(collided_output==0x8A000000);
    collided_output=0x12345678;
    for(unsigned raw=0;raw<256;++raw) {
        StormboltPowerCellRemoteCache wire{};wire.current[4]=std::uint32_t(raw)<<23;
        StormboltPowerCellRemoteFlags untouched{true,true,true,true};bool previous=true;
        assert(!decode_stormbolt_power_cell_sync(wire,untouched,previous));
        assert(untouched.empty && untouched.below && untouched.reached && untouched.effect && previous);
        wire.previous[4]=wire.current[4];wire.current[4]=0x88000000;
        assert(decode_stormbolt_power_cell_sync(wire,untouched,previous));
        assert(untouched.effect && !previous); // Raw native bit27 is not a prior extension event.
    }
    assert(!encode_stormbolt_power_cell_sync(0x80000000,{false,true,false,true},collided_output));
    assert(collided_output==0x12345678);
    for(unsigned raw=0;raw<256;++raw) for(unsigned mask=0;mask<16;++mask) {
        const std::uint32_t native=(std::uint32_t(raw)<<23)|0x0055AA55u;
        const StormboltPowerCellRemoteFlags sent{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8)};
        std::uint32_t encoded=0;assert(encode_stormbolt_power_cell_sync(native,sent,encoded));
        assert((encoded&0x00FFFFFFu)==(native&0x00FFFFFFu));
        assert((encoded&0x70000000u)==(native&0x70000000u));
        StormboltPowerCellRemoteCache wire{};wire.current[4]=encoded;
        StormboltPowerCellRemoteFlags received{};bool previous=true;
        assert(decode_stormbolt_power_cell_sync(wire,received,previous));
        assert(received.empty==sent.empty && received.below==sent.below && received.reached==sent.reached && received.effect==sent.effect && !previous);
    }
    const StormboltPowerCellSyncEventState send_event{true};
    for (std::uint32_t count=0;count<=5;++count) {
        const auto classified=classify_stormbolt_power_cell_sync(true,count,4,2,send_event);
        assert(classified.effect && classified.empty==(count<2));
        assert(classified.below==(count>=2 && count!=4) && classified.reached==(count==4));
    }
    const auto unpowered=classify_stormbolt_power_cell_sync(false,4,4,2,send_event);
    assert(unpowered.effect && !unpowered.empty && !unpowered.below && !unpowered.reached);
    const auto wrapped_band=classify_stormbolt_power_cell_sync(true,3,4,5,send_event);
    assert(wrapped_band.empty && !wrapped_band.below && !wrapped_band.reached);
    StormboltPowerCellSyncEventState sync_event{};
    assert(!snapshot_stormbolt_power_cell_sync_event(sync_event));
    mark_stormbolt_power_cell_sync_event(sync_event);
    mark_stormbolt_power_cell_sync_event(sync_event);
    assert(snapshot_stormbolt_power_cell_sync_event(sync_event));
    assert(snapshot_stormbolt_power_cell_sync_event(sync_event)); // Reading is not consumption.
    complete_stormbolt_power_cell_sync_producer(sync_event); // Unchanged/no-send also consumes.
    assert(!snapshot_stormbolt_power_cell_sync_event(sync_event));
    mark_stormbolt_power_cell_sync_event(sync_event);
    assert(snapshot_stormbolt_power_cell_sync_event(sync_event));
    complete_stormbolt_power_cell_sync_producer(sync_event);
    assert(!snapshot_stormbolt_power_cell_sync_event(sync_event));
    std::array<std::byte,0x59C4> remote_cache_fixture{};
    put(remote_cache_fixture.data(),0xDB0,reinterpret_cast<std::uintptr_t>(owner.data()));
    for (unsigned i=0;i<5;++i) {
        put(remote_cache_fixture.data(),0x5998+i*4,0x11110000u+i);
        put(remote_cache_fixture.data(),0x59B0+i*4,0x22220000u+i);
    }
    StormboltPowerCellRemoteCache cached{};
    assert(read_stormbolt_power_cell_remote_cache(remote_cache_fixture.data(),cached));
    assert(cached.actor==reinterpret_cast<std::uintptr_t>(owner.data()));
    for (unsigned i=0;i<5;++i)
        assert(cached.current[i]==0x11110000u+i && cached.previous[i]==0x22220000u+i);
    const auto saved_cache=cached;
    put(remote_cache_fixture.data(),0xDB0,std::uintptr_t{0});
    assert(!read_stormbolt_power_cell_remote_cache(remote_cache_fixture.data(),cached));
    assert(cached.actor==saved_cache.actor && cached.current==saved_cache.current && cached.previous==saved_cache.previous);
    // Exhaust the three classification bits, owner gate, edge and identity.
    for (unsigned mask=0;mask<16;++mask) {
        StormboltPowerCellRemoteFlags flags{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8)};
        for (unsigned conditions=0;conditions<32;++conditions) {
            const bool enabled=conditions&1, previous=conditions&2;
            const bool present=conditions&4, player=conditions&8, local=conditions&16;
            auto decision=decide_stormbolt_power_cell_remote(flags,previous,enabled,2,4,present,player,local);
            const unsigned expected=enabled ? 2u : flags.empty ? 0u : flags.below ? 3u : flags.reached ? 4u : 2u;
            const bool effect=flags.effect && !previous && present && player && !local;
            assert(decision.count==expected && decision.continue_effects==effect);
            assert(decision.reached_effect==(effect && expected==4));
        }
    }
    const auto zero_threshold=decide_stormbolt_power_cell_remote(
        {false,true,false,false},false,false,2,0,true,true,false);
    assert(zero_threshold.count==UINT32_MAX);
    const auto above=decide_stormbolt_power_cell_remote(
        {false,false,false,true},false,true,5,4,true,true,false);
    assert(above.continue_effects && !above.reached_effect);
    put(owner.data(),0x13D58,static_cast<void*>(&primary_first));
    StormboltPowerCellPostEffectsResult post_result{};
    assert(request_stormbolt_power_cell_post_effects(owner.data(),
        +[](void* p,std::uint32_t slot,std::uint32_t group,std::uint32_t record,bool post)->void* {
            assert(slot==1 && group==0x7D0 && record==post_requests && post);
            assert(p==(record==0 ? &primary_first : &primary_second));
            ++post_requests;
            put(owner.data(),0x13D58,static_cast<void*>(&primary_second));
            return record==0 ? nullptr : &primary_second;
        },post_result));
    assert(post_requests==2 && post_result.invoked[0] && post_result.invoked[1] &&
        !post_result.native_results[0] && post_result.native_results[1]==&primary_second);
    put(owner.data(),0x60,std::uint8_t{1});put(owner.data(),0x68,1.0f);
    put(owner.data(),0x126D8,static_cast<void*>(skills.data()));put(owner.data(),0x7D20,static_cast<void*>(status.data()));
    put(owner.data(),0x2B8,std::uint32_t{1});put(skills.data(),0,std::uintptr_t{1});put(skills.data(),0x141C,std::int32_t{3});
    put(resource.data(),0,std::uintptr_t{3});put(resource.data(),0xA8C,std::uint32_t{4});put(resource.data(),0xA90,std::uint32_t{2});
    auto slot=reinterpret_cast<std::uintptr_t>(resource.data());const auto n=bindings(&slot);
    std::array<unsigned char,7> original_status{},original_loop{},original_timer{};
    std::memcpy(original_status.data(),&stormbolt_status_test_patch_site,7);
    std::memcpy(original_loop.data(),&stormbolt_loop_test_patch_site,7);
    std::memcpy(original_timer.data(),&stormbolt_timer_test_patch_site,7);
    HookTransaction transaction;assert(transaction.begin()==MH_OK);
    StormboltPowerCellSession<2,2> session(n);
    assert(session.prepare(transaction,original_status,original_timer));
    assert(!std::memcmp(original_timer.data(),&stormbolt_timer_test_patch_site,7));
    assert(!session.activate_after_shared_commit()); // Queued lifecycle must activate first.
    assert(transaction.commit_pending_without_rollback()==MH_OK);
    assert(!std::memcmp(original_timer.data()+5,&stormbolt_timer_test_patch_site+5,2)); // Native TEST remains executable.
    assert(session.activate_after_shared_commit());
    auto* damage=damage_session_fixture::component.data();
    auto* absorption=damage_session_fixture::state_object.data();
    put(owner.data(),0x7658,static_cast<void*>(damage));
    put(absorption,0xDB0,static_cast<void*>(owner.data()));
    assert(damage_session_fixture::construct(damage)==damage);
    StormboltDamageExceptionToken damage_token{};
    assert(session.damage_states().token_for_live_address(reinterpret_cast<std::uintptr_t>(damage),damage_token));
    assert(damage_session_fixture::absorb(absorption,8.0f)==2.0f);
    StormboltDamageExceptionState damage_state{};
    assert(session.damage_states().snapshot(damage_token,damage_state) && damage_state.absorbed_positive_input);
    damage_session_fixture::update(damage);
    assert(session.damage_states().snapshot(damage_token,damage_state) && !damage_state.absorbed_positive_input);
    assert(damage_session_fixture::destroy(damage,0)==damage);
    assert(!session.damage_states().snapshot(damage_token,damage_state));
    assert(damage_session_fixture::construct(damage)==damage);
    put(damage,0x60,static_cast<void*>(owner.data()));
    assert(status_construct(status.data(),owner.data())==status.data());
    assert(loop_construct(component.data())==component.data());
    put(owner.data(),0x126C8,static_cast<void*>(component.data()));
    StormboltPowerCellToken st{};StormboltPowerCellLoopToken lt{};
    assert(session.statuses().token_for_live_address(reinterpret_cast<std::uintptr_t>(status.data()),st));
    StormboltPowerCellRemoteDecision remote{};
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.timer=123;return true;}));
    assert(commit_stormbolt_power_cell_remote(st,session.statuses(),
        {false,false,true,true},false,false,4,true,true,false,remote));
    assert(remote.count==4 && remote.reached_effect);
    StormboltPowerCellState remote_state{};
    assert(session.statuses().snapshot(st,remote_state) && remote_state.timer==123);
    assert(commit_stormbolt_power_cell_remote(st,session.statuses(),
        {false,true,false,true},true,false,4,true,true,false,remote));
    assert(remote.count==3 && !remote.continue_effects);
    auto stale=st; ++stale.generation;
    remote={99,true,true};
    assert(!commit_stormbolt_power_cell_remote(stale,session.statuses(),
        {true,false,false,true},false,false,4,true,true,false,remote));
    assert(remote.count==99 && remote.continue_effects && remote.reached_effect);
    assert(session.statuses().snapshot(st,remote_state) && remote_state.count==3);
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.timer=-1;return true;}));
    assert(session.loops().token_for_live_address(reinterpret_cast<std::uintptr_t>(component.data()),lt));
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.count=1;return true;}));
    std::array<std::byte,0xC0> hit{};
    std::array<std::byte,0x1A0> attack{};
    float amount=1;
    put(hit.data(),0xB8,static_cast<void*>(attack.data()));
    put(attack.data(),0x20,1.0f);
    StormboltPowerCellGrowthInputs growth{};
    assert(read_stormbolt_power_cell_growth_inputs(damage,hit.data(),&amount,55,session.damage_states(),
        session.statuses(),n.loop_inputs,n.skill_value_query,growth));
    assert(growth.gate.first_actor_powered && growth.gate.associated_actor_powered &&
        growth.associated_actor==owner.data() && growth.gate.count==1 && growth.gate.threshold==4 &&
        !growth.gate.associated_actor_is_local);
    int local_manager=0;void* manager_pointer=&local_manager;
    StormboltPowerCellRemoteInputs native_remote{};
    assert(read_stormbolt_power_cell_remote_inputs(saved_cache,{false,true,false,true},false,
        session.statuses(),n.loop_inputs,native_remote));
    assert(native_remote.status_token.generation==st.generation && native_remote.threshold==4 &&
        native_remote.associated==owner.data() && native_remote.associated_player);
    assert(resolve_stormbolt_power_cell_remote_identity(native_remote,session.statuses(),&manager_pointer,
        +[](void*)->void* {return owner.data();}) && native_remote.associated_local);
    assert(resolve_stormbolt_power_cell_remote_identity(native_remote,session.statuses(),&manager_pointer,
        +[](void*)->void* {return nullptr;}) && !native_remote.associated_local);
    assert(resolve_stormbolt_power_cell_growth_local_identity(growth,session.statuses(),&manager_pointer,
        +[](void*)->void* {return owner.data();}));
    assert(growth.gate.associated_actor_is_local);
    assert(resolve_stormbolt_power_cell_growth_local_identity(growth,session.statuses(),&manager_pointer,
        +[](void*)->void* {return nullptr;}));
    assert(!growth.gate.associated_actor_is_local);
    growth.gate.associated_actor_is_local=true;
    growth.gate.owner_enabled=true;
    growth.duration=150.0f;
    StormboltPowerCellGrowthResult growth_result{};
    for (unsigned i=2;i<=4;++i) {
        assert(commit_stormbolt_power_cell_growth(growth,session.statuses(),growth_result));
        StormboltPowerCellState increased{};
        assert(session.statuses().snapshot(st,increased) && increased.count==i);
        assert(growth_result==(i<4 ? StormboltPowerCellGrowthResult::below_threshold :
            StormboltPowerCellGrowthResult::reached_threshold));
        assert(increased.timer==(i<4 ? -1.0f : 150.0f));
        assert(apply_stormbolt_power_cell_growth_effects(growth_result,owner.data(),session.loops(),
            stormbolt_loop_component_bindings(n).audio,n.skill_prompt));
        StormboltPowerCellLoopState grown_loop{};
        assert(session.loops().snapshot(lt,grown_loop));
        if (i<4) assert(grown_loop.growth && prompts==0);
        else assert(grown_loop.timer==5 && !grown_loop.active && prompts==1);
    }
    assert(commit_stormbolt_power_cell_growth(growth,session.statuses(),growth_result) &&
        growth_result==StormboltPowerCellGrowthResult::unchanged);
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.count=1;s.timer=-1;return true;}));
    assert(damage_session_fixture::construct(damage)==damage);
    put(damage,0x60,static_cast<void*>(owner.data()));
    put(owner.data(),0x8938,std::uint8_t{1});
    put(resource.data(),0xA94,175.0f);
    StormboltPowerCellGrowthBindings growth_bindings{55,stormbolt_loop_component_bindings(n).audio,
        n.skill_prompt,&manager_pointer,+[](void*)->void* {return owner.data();}};
    for (unsigned i=2;i<=4;++i) {
        assert(process_stormbolt_power_cell_growth_event(damage,hit.data(),&amount,session.damage_states(),
            session.statuses(),session.loops(),growth_bindings,growth_result));
        StormboltPowerCellState increased{};
        assert(session.statuses().snapshot(st,increased) && increased.count==i);
        if (i==4) assert(increased.timer==175 && prompts==2);
    }
    assert(damage_session_fixture::destroy(damage,0)==damage);
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.count=1;s.timer=-1;return true;}));
    assert(damage_session_fixture::construct(damage)==damage);
    put(damage,0x60,static_cast<void*>(owner.data()));
    std::array<std::byte,0x170> visual_actor{};
    int sound=0;
    std::array<float,3> first{0,0,0},second{0,0,1};
    put(visual_actor.data(),0xC,std::uint8_t{0xE});
    put(component.data(),0xF0,static_cast<void*>(visual_actor.data()));
    put(owner.data(),0xA18,static_cast<void*>(&sound));
    growth_bindings.loop.native_first=first.data();growth_bindings.loop.native_second=second.data();
    growth_bindings.loop.container_at=+[](void* value)->void* {return value;};
    growth_bindings.loop.request=+[](void*,std::int32_t category,const char* event,StormboltNamedEventParameters* p) {
        assert(prompts==3 && category==-1 && !std::strcmp(event,"PLAY_PL_BODY_068_DMG_R_LP"));
        assert(p->actor && p->flags==0x10);++plays;return false;
    };
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.count=3;return true;}));
    assert(process_stormbolt_power_cell_growth_event(damage,hit.data(),&amount,session.damage_states(),
        session.statuses(),session.loops(),growth_bindings,growth_result));
    StormboltPowerCellLoopState played{};
    assert(session.loops().snapshot(lt,played) && played.active && played.timer==5 && plays==1);
    unsigned remote_bodies=0;
    auto remote_body=+[](void* primary,std::uint32_t slot,std::uint32_t group,
        std::uint32_t record,bool post)->void* {
        assert(primary==&primary_second && slot==1 && group==0x7D0 && record<2 && post);
        assert(prompts==3 && plays==2);
        return nullptr;
    };
    StormboltPowerCellPostEffectsResult remote_post{};
    assert(apply_stormbolt_power_cell_remote_effects({4,true,true},owner.data(),session.loops(),
        growth_bindings.loop,remote_body,remote_post));
    remote_bodies=unsigned(remote_post.invoked[0])+unsigned(remote_post.invoked[1]);
    assert(remote_bodies==2 && plays==2 && prompts==3);
    assert(session.loops().transact(lt,[](auto& s) noexcept {s.growth=false;return true;}));
    assert(apply_stormbolt_power_cell_remote_effects({3,true,false},owner.data(),session.loops(),
        growth_bindings.loop,remote_body,remote_post));
    assert(session.loops().snapshot(lt,played) && played.growth && plays==2 && prompts==3);
    assert(apply_stormbolt_power_cell_remote_effects({3,false,false},nullptr,session.loops(),
        growth_bindings.loop,nullptr,remote_post));
    assert(!remote_post.invoked[0] && !remote_post.invoked[1] && plays==2);
    StormboltPowerCellRemoteInputs remote_inputs{st,{false,false,true,true},true,
        false,4,owner.data(),true,false};
    assert(process_stormbolt_power_cell_remote(remote_inputs,session.statuses(),session.loops(),
        growth_bindings.loop,remote_body,remote,remote_post));
    assert(remote.count==4 && !remote.continue_effects && !remote_post.invoked[0] && plays==2);
    remote_inputs.flags={true,false,false,true};remote_inputs.previous_effect=false;
    assert(!process_stormbolt_power_cell_remote(remote_inputs,session.statuses(),session.loops(),
        growth_bindings.loop,nullptr,remote,remote_post));
    assert(remote.count==0 && remote.continue_effects && !remote.reached_effect);
    assert(session.statuses().snapshot(st,remote_state) && remote_state.count==0);
    remote_inputs.status_token=stale;remote={99,true,true};remote_post.invoked={true,true};
    assert(!process_stormbolt_power_cell_remote(remote_inputs,session.statuses(),session.loops(),
        growth_bindings.loop,remote_body,remote,remote_post));
    assert(remote.count==99 && remote_post.invoked[0] && remote_post.invoked[1] && plays==2);
    assert(process_stormbolt_power_cell_remote_native(saved_cache,{false,false,true,true},false,
        session.statuses(),session.loops(),n.loop_inputs,growth_bindings.loop,&manager_pointer,
        +[](void*)->void* {return owner.data();},remote_body,remote,remote_post));
    assert(!remote.continue_effects && !remote_post.invoked[0] && plays==2);
    assert(process_stormbolt_power_cell_remote_native(saved_cache,{false,true,false,true},true,
        session.statuses(),session.loops(),n.loop_inputs,growth_bindings.loop,nullptr,nullptr,
        nullptr,remote,remote_post));
    assert(!remote.continue_effects && !remote_post.invoked[0] && plays==2);
    remote={99,true,true};
    assert(!process_stormbolt_power_cell_remote_native(saved_cache,{false,true,false,true},false,
        session.statuses(),session.loops(),n.loop_inputs,growth_bindings.loop,nullptr,nullptr,
        remote_body,remote,remote_post));
    assert(remote.count==0 && !remote.continue_effects && plays==2);
    assert(damage_session_fixture::destroy(damage,0)==damage);
    put(component.data(),0xF0,static_cast<void*>(nullptr));
    assert(session.loops().transact(lt,[](auto& s) noexcept {s.active=false;s.timer=0;return true;}));
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.count=1;s.timer=-1;return true;}));
    assert(damage_session_fixture::destroy(damage,0)==damage);
    assert(session.loops().transact(lt,[](auto& s) noexcept {s.growth=true;return true;}));
    std::array<std::uint64_t,24> output{};
    stormbolt_status_test_call(status.data(),output.data());
    StormboltPowerCellState state{};assert(session.statuses().snapshot(st,state) && state.event_eligible && flags==1);
    {
        auto scoped=n.loop_inputs;scoped.target_weapon_vtable=0x12345678;
        std::array<unsigned char,0xA00> equipped{};
        put(owner.data(),0x76B0,equipped.data());
        put(equipped.data(),0,std::uintptr_t{0x12345678});
        put(equipped.data(),0x9B0,owner.data());
        put(equipped.data(),0x9F8,std::uint32_t{1}); // Non-bowgun, same fake VT to isolate category.
        put(equipped.data(),0x9FC,std::uint32_t{238});
        const auto before_flags=flags;
        assert(!update_stormbolt_power_cell_event_eligibility_from_status(status.data(),session.statuses(),scoped,
            n.skill_value_query,n.special_to_normal.action.actor_flag));
        assert(flags==before_flags);
        put(equipped.data(),0x9F8,std::uint32_t{13});put(equipped.data(),0x9FC,std::uint32_t{237});
        assert(!update_stormbolt_power_cell_event_eligibility_from_status(status.data(),session.statuses(),scoped,
            n.skill_value_query,n.special_to_normal.action.actor_flag) && flags==before_flags);
        for(std::uint32_t id:{238u,239u}) {
            put(equipped.data(),0x9FC,id);
            assert(update_stormbolt_power_cell_event_eligibility_from_status(status.data(),session.statuses(),scoped,
                n.skill_value_query,n.special_to_normal.action.actor_flag));
        }
        assert(flags==before_flags+2);
        put(owner.data(),0x76B0,static_cast<void*>(nullptr));
    }

    stormbolt_loop_test_call(component.data(),output.data());
    StormboltPowerCellLoopState loop{};
    assert(session.loops().snapshot(lt,loop) && loop.visual_phase==1 && !loop.growth && parameters==3);
    put(owner.data(),0x8938,std::uint8_t{1});
    assert(session.statuses().transact(st,[](auto& s) noexcept {s.timer=1;return true;}));
    stormbolt_timer_test_call(status.data(),output.data());
    assert(session.statuses().snapshot(st,state) && state.timer==0.75f && state.count==1);
    assert(stormbolt_timer_test_query_owner==owner.data());
    status_reset(status.data(),false);assert(!session.statuses().snapshot(st,state));
    assert(session.statuses().token_for_live_address(reinterpret_cast<std::uintptr_t>(status.data()),st));
    assert(session.statuses().snapshot(st,state) && state.count==0 && !state.event_eligible);
    status_destroy(status.data());loop_destroy(component.data(),5);
    assert(!session.statuses().snapshot(st,state) && !session.loops().snapshot(lt,loop));
    assert(!session.release_after_shared_stop());
    assert(session.restore_updates());
    assert(transaction.stop()==MH_OK && session.release_after_shared_stop());
    assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
    assert(!std::memcmp(original_loop.data(),&stormbolt_loop_test_patch_site,7));
    status_construct(status.data(),owner.data());loop_construct(component.data());
    assert(!session.statuses().token_for_live_address(reinterpret_cast<std::uintptr_t>(status.data()),st));
    assert(!session.loops().token_for_live_address(reinterpret_cast<std::uintptr_t>(component.data()),lt));
    assert(!std::memcmp(original_timer.data(),&stormbolt_timer_test_patch_site,7));
    // The unchanged tail is still owned: a post-prepare edit must reject the
    // shared commit inside Freeze, retain it, and permit exact recovery.
    {
        HookTransaction tail_transaction;assert(tail_transaction.begin()==MH_OK);
        tail_transaction.retain_failed_adds_for_coordinator();
        StormboltPowerCellSession<2,2> tail_session(n);
        assert(tail_session.prepare(tail_transaction,original_status,original_timer));
        auto* tail=&stormbolt_status_test_patch_site+6;DWORD old=0,unused=0;
        assert(VirtualProtect(tail,1,PAGE_EXECUTE_READWRITE,&old));*tail^=1;
        assert(VirtualProtect(tail,1,old,&unused));
        assert(tail_transaction.commit_pending_without_rollback()!=MH_OK);
        assert(!tail_session.restore_updates());
        assert(*tail==static_cast<unsigned char>(original_status[6]^1));
        assert(VirtualProtect(tail,1,PAGE_EXECUTE_READWRITE,&old));*tail=original_status[6];
        assert(VirtualProtect(tail,1,old,&unused));
        assert(FlushInstructionCache(GetCurrentProcess(),tail,1));
        assert(tail_session.restore_updates());
        assert(tail_transaction.stop()==MH_OK && tail_session.release_after_shared_stop());
        assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
    }
    // Wrong loop span is rejected during prepare; queued dependencies restore.
    auto faulty=n;faulty.loop_update_site=faulty.power_cell_status_update_site;
    HookTransaction rollback;assert(rollback.begin()==MH_OK);
    StormboltPowerCellSession<2,2> recovery(faulty);
    assert(!recovery.prepare(rollback,original_status,original_timer));assert(recovery.restore_updates());
    assert(rollback.stop()==MH_OK && recovery.release_after_shared_stop());
    assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
    auto timer_fault=n;timer_fault.power_cell_timer_update_site=timer_fault.power_cell_status_update_site;
    HookTransaction timer_rollback;assert(timer_rollback.begin()==MH_OK);
    StormboltPowerCellSession<2,2> timer_recovery(timer_fault);
    assert(!timer_recovery.prepare(timer_rollback,original_status,original_timer)); // Wrong CALL rejected before publication.
    assert(timer_recovery.restore_updates());
    assert(timer_rollback.stop()==MH_OK && timer_recovery.release_after_shared_stop());
    assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
    assert(!std::memcmp(original_loop.data(),&stormbolt_loop_test_patch_site,7));
    assert(!std::memcmp(original_timer.data(),&stormbolt_timer_test_patch_site,7));
    auto* remote_site=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    assert(remote_site);
    const std::array<unsigned char,6> remote_original{{0x8B,0x87,0x98,0x59,0,0}};
    std::memcpy(remote_site,remote_original.data(),6);
    StormboltRemoteUpdateBindings remote_bindings{};
    remote_bindings.inputs=n.loop_inputs;remote_bindings.audio=growth_bindings.loop;
    remote_bindings.manager_slot=&manager_pointer;
    remote_bindings.local_player=+[](void*)->void* {return nullptr;};
    remote_bindings.equipment=+[](void*,std::uint32_t,std::uint32_t,std::uint32_t,bool)->void* {return nullptr;};
    remote_bindings.decode=+[](const StormboltPowerCellRemoteCache&,StormboltPowerCellRemoteFlags&,bool&) {return false;};
    for (bool failure:{false,true}) {
        auto with_remote=n;
        with_remote.power_cell_remote_update_site=failure ? n.power_cell_status_update_site : reinterpret_cast<std::uintptr_t>(remote_site);
        const auto old_remote_dispatch=stormbolt_remote_update_dispatch_target;
        HookTransaction remote_transaction;assert(remote_transaction.begin()==MH_OK);
        StormboltPowerCellSession<2,2> remote_session(with_remote,remote_bindings);
        assert(remote_session.prepare(remote_transaction,original_status,original_timer)==!failure);
        if(failure) {
            assert(remote_session.restore_updates());
            assert(remote_transaction.stop()==MH_OK && remote_session.release_after_shared_stop());
            continue;
        }
        assert(stormbolt_remote_update_dispatch_target!=old_remote_dispatch);
        assert(remote_transaction.commit_pending_without_rollback()==MH_OK);
        assert(remote_session.activate_after_shared_commit()==!failure);
        std::array<std::byte,0xDB8> master{};
        assert(sync_construct(master.data())==master.data());
        put(master.data(),0xDB0,reinterpret_cast<std::uintptr_t>(owner.data()));
        put(owner.data(),0x14F8,reinterpret_cast<std::uintptr_t>(master.data()));
        StormboltPowerCellSyncToken tracked{};
        assert(capture_stormbolt_power_cell_sync_controller(owner.data(),77,remote_session.sync_events(),tracked));
        assert(mark_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
        assert(complete_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
        assert(mark_stormbolt_power_cell_sync_after_growth(StormboltPowerCellGrowthResult::below_threshold,
            owner.data(),&manager_pointer,+[](void*)->void* {return nullptr;},77,remote_session.sync_events()));
        StormboltPowerCellSyncEventState sync_after{};
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && !sync_after.pending);
        assert(mark_stormbolt_power_cell_sync_after_growth(StormboltPowerCellGrowthResult::reached_threshold,
            owner.data(),&manager_pointer,+[](void*)->void* {return owner.data();},77,remote_session.sync_events()));
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && sync_after.pending);
        assert(mark_stormbolt_power_cell_sync_after_growth(StormboltPowerCellGrowthResult::unchanged,
            nullptr,nullptr,nullptr,0,remote_session.sync_events()));
        assert(status_construct(status.data(),owner.data())==status.data());
        assert(loop_construct(component.data())==component.data());
        auto* full_damage=damage_session_fixture::component.data();
        assert(damage_session_fixture::construct(full_damage)==full_damage);
        put(full_damage,0x60,static_cast<void*>(owner.data()));
        StormboltPowerCellToken full_status{};
        assert(remote_session.statuses().token_for_live_address(reinterpret_cast<std::uintptr_t>(status.data()),full_status));
        assert(remote_session.statuses().transact(full_status,[](auto& s) noexcept {s.count=1;s.event_eligible=true;return true;}));
        assert(complete_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
        auto complete_growth=growth_bindings;
        complete_growth.local_player=+[](void*)->void* {return owner.data();};
        complete_sync_store=&remote_session.sync_events();complete_body_requests=0;expected_sync_pending=true;
        StormboltPowerCellCompleteGrowthBindings complete_bindings{complete_growth,77,
            +[](void* primary,std::uint32_t slot,std::uint32_t group,std::uint32_t record,bool post)->void* {
                assert(primary==&primary_second && slot==1 && group==0x7D0 && record==complete_body_requests%2 && post);
                StormboltPowerCellSyncToken token{};
                assert(capture_stormbolt_power_cell_sync_controller(owner.data(),77,*complete_sync_store,token));
                StormboltPowerCellSyncEventState value{};
                assert(complete_sync_store->snapshot(token,value) && value.pending==expected_sync_pending);
                ++complete_body_requests;return nullptr;
            }};
        StormboltPowerCellPostEffectsResult full_effects{};
        assert(process_stormbolt_power_cell_complete_growth_event(full_damage,hit.data(),&amount,
            remote_session.damage_states(),remote_session.statuses(),remote_session.loops(),remote_session.sync_events(),
            complete_bindings,growth_result,full_effects));
        assert(growth_result==StormboltPowerCellGrowthResult::below_threshold && full_effects.invoked[0] && full_effects.invoked[1]);
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && sync_after.pending);
        assert(complete_body_requests==2);
        const auto previous_growth=stormbolt_growth_dispatch_target;
        StormboltGrowthDispatch<2,2,2,2> growth_dispatch(remote_session.statuses(),remote_session.damage_states(),
            remote_session.loops(),remote_session.sync_events(),complete_bindings);
        assert(growth_dispatch.publish() && !growth_dispatch.publish());
        assert(remote_session.statuses().transact(full_status,[](auto& s) noexcept {s.count=1;return true;}));
        stormbolt_growth_dispatch_target(full_damage,hit.data(),&amount);
        StormboltPowerCellState dispatched{};
        assert(remote_session.statuses().snapshot(full_status,dispatched) && dispatched.count==2);
        assert(complete_body_requests==4);
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && sync_after.pending);
        assert(growth_dispatch.withdraw() && stormbolt_growth_dispatch_target==previous_growth);
        complete_body_requests=2; // Keep the following independent scenario counters.

        const std::array<unsigned char,8> growth_negative{{0x85,0xF6,0x0F,0x88,0x96,0,0,0}};
        const std::array<unsigned char,9> growth_post{{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76}};
        const std::array<unsigned char,12> growth_remaining{{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0}};
        for(unsigned conflict=0;conflict<4;++conflict) {
            std::array<std::uintptr_t,3> sites{{reinterpret_cast<std::uintptr_t>(remote_site+192),
                reinterpret_cast<std::uintptr_t>(remote_site+224),reinterpret_cast<std::uintptr_t>(remote_site+256)}};
            std::memcpy(remote_site+192,growth_negative.data(),8);
            std::memcpy(remote_site+224,growth_post.data(),9);
            std::memcpy(remote_site+256,growth_remaining.data(),12);
            if(conflict) *reinterpret_cast<unsigned char*>(sites[conflict-1])=0xCC;
            const auto previous=stormbolt_growth_dispatch_target;
            StormboltGrowthSession<2,2> growth_session(sites,remote_session.statuses(),remote_session.damage_states(),
                remote_session.loops(),remote_session.sync_events(),complete_bindings);
            HookTransaction growth_transaction;assert(growth_transaction.begin_shared_with(remote_transaction,true)==MH_OK);
            assert(growth_session.prepare(&growth_transaction)==(conflict==0));
            if(!conflict) {
                assert(growth_transaction.commit_pending_without_rollback()==MH_OK);
                assert(growth_session.activate());
            }
            assert(growth_session.restore() && !growth_session.active());
            assert(growth_transaction.stop()==MH_OK && growth_session.release_after_shared_stop());
            assert(stormbolt_growth_dispatch_target==previous);
            for(unsigned n=0;n<3;++n) {
                if(conflict==n+1) assert(*reinterpret_cast<unsigned char*>(sites[n])==0xCC);
                else if(n==0) assert(!std::memcmp(reinterpret_cast<void*>(sites[n]),growth_negative.data(),8));
                else if(n==1) assert(!std::memcmp(reinterpret_cast<void*>(sites[n]),growth_post.data(),9));
                else assert(!std::memcmp(reinterpret_cast<void*>(sites[n]),growth_remaining.data(),12));
            }
        }


        StormboltPowerCellSyncSnapshot send_snapshot{};
        assert(read_stormbolt_power_cell_sync_snapshot(master.data(),77,remote_session.sync_events(),
            remote_session.statuses(),n.loop_inputs,n.skill_value_query,send_snapshot));
        assert(send_snapshot.controller_token.generation==tracked.generation && send_snapshot.flags.below &&
            !send_snapshot.flags.empty && !send_snapshot.flags.reached && send_snapshot.flags.effect);
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && sync_after.pending);
        StormboltPowerCellSyncFrames<2> native_frames;
        StormboltSyncPackBindings pack_bindings{77,n.loop_inputs,n.skill_value_query,
            &encode_stormbolt_power_cell_sync};
        StormboltSyncPackDispatch<2,2,2> pack_dispatch(native_frames,remote_session.sync_events(),remote_session.statuses(),pack_bindings);
        StormboltSyncCompleteDispatch<2,2> consume_dispatch(native_frames,remote_session.sync_events());
        assert(pack_dispatch.publish() && consume_dispatch.publish());
        int native_frame=0;const auto frame_address=reinterpret_cast<std::uintptr_t>(&native_frame);
        assert(stormbolt_sync_pack_dispatch_target(master.data(),frame_address,0x11)==0x8A000011u);
        assert(native_frames.active() && !pack_dispatch.withdraw() && !consume_dispatch.withdraw());
        // Failed retirement closes new pack calls but keeps completion alive.
        assert(stormbolt_sync_pack_dispatch_target(master.data(),frame_address+8,0x11)==0x11);
        stormbolt_sync_complete_dispatch_target(master.data(),frame_address);
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && !sync_after.pending);
        assert(!native_frames.active());
        assert(pack_dispatch.withdraw() && pack_dispatch.publish());
        StormboltPowerCellStore<2> receiver_statuses;
        StormboltPowerCellLoopStore<2> receiver_loops;
        // Native objects are already constructed above; these stores model a separate peer.
        const auto receiver_token=receiver_statuses.initialize(reinterpret_cast<std::uintptr_t>(status.data()));
        const auto receiver_loop=receiver_loops.initialize(reinterpret_cast<std::uintptr_t>(component.data()));
        assert(receiver_token && receiver_loop);
        static unsigned wire_bodies=0;
        auto wire_body=+[](void*,std::uint32_t slot,std::uint32_t group,std::uint32_t record,bool post)->void* {
            assert(slot==1 && group==0x7D0 && record<2 && post);++wire_bodies;return nullptr;
        };
        for(unsigned source_count:{1u,2u,4u}) {
            assert(remote_session.statuses().transact(full_status,[source_count](auto& s) noexcept {s.count=source_count;return true;}));
            StormboltPowerCellRemoteCache received_cache{};received_cache.actor=reinterpret_cast<std::uintptr_t>(owner.data());
            received_cache.previous[4]=0x7F800011; // Noncanonical original first history, no extension marker.
            wire_bodies=0;
            for(bool event:{true,false,true}) {
                if(event) assert(mark_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
                received_cache.current[4]=stormbolt_sync_pack_dispatch_target(master.data(),frame_address,0x7F800011);
                assert(received_cache.current[4]&stormbolt_power_cell_sync_marker);
                stormbolt_sync_complete_dispatch_target(master.data(),frame_address);
                assert(remote_session.sync_events().snapshot(tracked,sync_after) && !sync_after.pending);
                StormboltPowerCellRemoteFlags wire_flags{};bool prior=false;
                assert(decode_stormbolt_power_cell_sync(received_cache,wire_flags,prior));
                put(owner.data(),0x8938,std::uint8_t{0});
                StormboltPowerCellRemoteDecision applied{};StormboltPowerCellPostEffectsResult applied_effects{};
                assert(process_stormbolt_power_cell_remote_native(received_cache,wire_flags,prior,
                    receiver_statuses,receiver_loops,n.loop_inputs,growth_bindings.loop,&manager_pointer,
                    +[](void*)->void* {return nullptr;},wire_body,applied,applied_effects));
                const unsigned expected_count=source_count==1?0:source_count==2?3:4;
                assert(applied.count==expected_count && applied.continue_effects==event);
                StormboltPowerCellState restored{};
                assert(receiver_statuses.snapshot(receiver_token,restored) && restored.count==expected_count);
                received_cache.previous=received_cache.current; // Native prehistory tail, after application.
                assert(decode_stormbolt_power_cell_sync(received_cache,wire_flags,prior));
                const auto before_duplicate=wire_bodies;
                assert(process_stormbolt_power_cell_remote_native(received_cache,wire_flags,prior,
                    receiver_statuses,receiver_loops,n.loop_inputs,growth_bindings.loop,&manager_pointer,
                    +[](void*)->void* {return nullptr;},wire_body,applied,applied_effects));
                assert(!applied.continue_effects && wire_bodies==before_duplicate);
                put(owner.data(),0x8938,std::uint8_t{1});
            }
            assert(wire_bodies==4); // Two event edges, two body requests each.
        }
        StormboltPowerCellLoopState wire_loop{};
        assert(receiver_loops.snapshot(receiver_loop,wire_loop) && wire_loop.timer==5);
        assert(remote_session.statuses().transact(full_status,[](auto& s) noexcept {s.count=2;return true;}));
        assert(pack_dispatch.withdraw() && consume_dispatch.withdraw());
        const std::array<unsigned char,6> producer_original{{0x89,0x7D,0xA7,0x4C,0x2B,0xC0}};
        const std::array<unsigned char,8> consume_original{{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0}};
        for (bool pack_failure:{false,true}) {
            std::memcpy(remote_site+64,producer_original.data(),6);
            std::memcpy(remote_site+128,consume_original.data(),8);
            if (pack_failure) remote_site[64]=0xCC;
            const auto previous_pack=stormbolt_sync_pack_dispatch_target;
            const auto previous_complete=stormbolt_sync_complete_dispatch_target;
            StormboltSyncProducerSession<2> producer(reinterpret_cast<std::uintptr_t>(remote_site+64),
                reinterpret_cast<std::uintptr_t>(remote_site+128),remote_session.sync_events(),remote_session.statuses(),pack_bindings);
            HookTransaction producer_transaction;
            assert(producer_transaction.begin_shared_with(remote_transaction,true)==MH_OK);
            assert(producer.prepare(&producer_transaction)==!pack_failure);
            if(!pack_failure) {
                assert(producer_transaction.commit_pending_without_rollback()==MH_OK);
                assert(producer.activate());
                assert(remote_site[128]==0xE8);
            } else assert(!std::memcmp(remote_site+128,consume_original.data(),8));
            if(!pack_failure) {
                (void)stormbolt_sync_pack_dispatch_target(master.data(),frame_address,0x11);
                assert(!producer.restore_patches() && producer.active());
                assert(!std::memcmp(remote_site+64,producer_original.data(),6));
                assert(remote_site[128]==0xE8); // Pending frame can still complete.
                stormbolt_sync_complete_dispatch_target(master.data(),frame_address);
            }
            assert(producer.restore() && !producer.active());
            assert(producer_transaction.stop()==MH_OK && producer.release_after_shared_stop());
            assert(stormbolt_sync_pack_dispatch_target==previous_pack && stormbolt_sync_complete_dispatch_target==previous_complete);
            assert(!std::memcmp(remote_site+128,consume_original.data(),8));
            if (!pack_failure) assert(!std::memcmp(remote_site+64,producer_original.data(),6));
            else assert(remote_site[64]==0xCC); // Do not overwrite the conflicting site.
        }
        assert(complete_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
        expected_sync_pending=false;
        assert(remote_session.statuses().transact(full_status,[](auto& s) noexcept {s.count=4;return true;}));
        assert(process_stormbolt_power_cell_complete_growth_event(full_damage,hit.data(),&amount,
            remote_session.damage_states(),remote_session.statuses(),remote_session.loops(),remote_session.sync_events(),
            complete_bindings,growth_result,full_effects));
        assert(growth_result==StormboltPowerCellGrowthResult::unchanged && full_effects.invoked[0] && full_effects.invoked[1]);
        assert(remote_session.sync_events().snapshot(tracked,sync_after) && !sync_after.pending);
        assert(read_stormbolt_power_cell_sync_snapshot(master.data(),77,remote_session.sync_events(),
            remote_session.statuses(),n.loop_inputs,n.skill_value_query,send_snapshot));
        assert(send_snapshot.flags.reached && !send_snapshot.flags.below && !send_snapshot.flags.effect);
        assert(complete_body_requests==4);
        assert(remote_session.statuses().transact(full_status,[](auto& s) noexcept {s.count=1;return true;}));
        complete_bindings.growth.local_player=+[](void*)->void* {return nullptr;};
        assert(process_stormbolt_power_cell_complete_growth_event(full_damage,hit.data(),&amount,
            remote_session.damage_states(),remote_session.statuses(),remote_session.loops(),remote_session.sync_events(),
            complete_bindings,growth_result,full_effects));
        StormboltPowerCellState not_local{};
        assert(remote_session.statuses().snapshot(full_status,not_local) && not_local.count==1);
        assert(growth_result==StormboltPowerCellGrowthResult::unchanged && complete_body_requests==6);
        put(attack.data(),0x20,0.0f);put(attack.data(),0xA0,std::uint32_t{7});
        assert(process_stormbolt_power_cell_complete_growth_event(full_damage,hit.data(),&amount,
            remote_session.damage_states(),remote_session.statuses(),remote_session.loops(),remote_session.sync_events(),
            complete_bindings,growth_result,full_effects));
        assert(!full_effects.invoked[0] && !full_effects.invoked[1] && complete_body_requests==6);
        put(attack.data(),0x20,1.0f);put(attack.data(),0xA0,std::uint32_t{0});
        complete_sync_store=nullptr;
        assert(damage_session_fixture::destroy(full_damage,0)==full_damage);
        status_destroy(status.data());loop_destroy(component.data(),5);
        assert(sync_destroy(master.data(),0)==master.data());
        assert(!read_stormbolt_power_cell_sync_snapshot(master.data(),77,remote_session.sync_events(),
            remote_session.statuses(),n.loop_inputs,n.skill_value_query,send_snapshot));
        assert(send_snapshot.flags.reached && send_snapshot.controller_token.generation==tracked.generation);
        assert(!mark_stormbolt_power_cell_sync_controller(tracked,remote_session.sync_events()));
        if (!failure) assert(remote_site[0]==0xE8 && remote_site[5]==remote_original[5]);
        assert(remote_session.restore_updates());
        assert(stormbolt_remote_update_dispatch_target==old_remote_dispatch);
        assert(remote_transaction.stop()==MH_OK && remote_session.release_after_shared_stop());
        assert(!std::memcmp(remote_site,remote_original.data(),6));
        assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
        assert(!std::memcmp(original_loop.data(),&stormbolt_loop_test_patch_site,7));
        assert(!std::memcmp(original_timer.data(),&stormbolt_timer_test_patch_site,7));
    }
    for (bool pack_failure:{false,true}) {
        const std::array<unsigned char,6> pack_bytes{{0x89,0x7D,0xA7,0x4C,0x2B,0xC0}};
        const std::array<unsigned char,8> complete_bytes{{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0}};
        std::memcpy(remote_site+64,pack_bytes.data(),6);
        std::memcpy(remote_site+128,complete_bytes.data(),8);
        if (pack_failure) remote_site[64]=0xCC;
        auto native_producer=n;
        native_producer.power_cell_sync_pack_site=reinterpret_cast<std::uintptr_t>(remote_site+64);
        native_producer.power_cell_sync_complete_site=reinterpret_cast<std::uintptr_t>(remote_site+128);
        StormboltSyncPackBindings pack_bindings{77,n.loop_inputs,n.skill_value_query,&encode_stormbolt_power_cell_sync};
        const auto previous_pack=stormbolt_sync_pack_dispatch_target;
        const auto previous_complete=stormbolt_sync_complete_dispatch_target;
        HookTransaction transaction;assert(transaction.begin()==MH_OK);
        StormboltPowerCellSession<2,2> combined(native_producer,{},pack_bindings);
        transaction.retain_failed_adds_for_coordinator();
        assert(combined.prepare(transaction,original_status,original_timer)==!pack_failure);
        if(!pack_failure) {
            assert(transaction.commit_pending_without_rollback()==MH_OK);
            assert(combined.activate_after_shared_commit());
            assert(remote_site[128]==0xE8);
        } else assert(!std::memcmp(remote_site+128,complete_bytes.data(),8));
        assert(combined.restore_updates());
        assert(transaction.stop()==MH_OK && combined.release_after_shared_stop());
        assert(stormbolt_sync_pack_dispatch_target==previous_pack && stormbolt_sync_complete_dispatch_target==previous_complete);
        assert(!std::memcmp(remote_site+128,complete_bytes.data(),8));
        if (pack_failure) assert(remote_site[64]==0xCC);
        else assert(!std::memcmp(remote_site+64,pack_bytes.data(),6));
        assert(!std::memcmp(original_status.data(),&stormbolt_status_test_patch_site,7));
        assert(!std::memcmp(original_loop.data(),&stormbolt_loop_test_patch_site,7));
        assert(!std::memcmp(original_timer.data(),&stormbolt_timer_test_patch_site,7));
    }
    assert(VirtualFree(remote_site,0,MEM_RELEASE));
    StormboltPowerCellSyncStore<2> sync_store;
    StormboltLifecycleHooks<2,StormboltPowerCellSyncEventState,StormboltPowerCellSyncToken> sync_lifecycle(sync_store);
    HookTransaction sync_transaction;assert(sync_transaction.begin()==MH_OK);
    assert(sync_lifecycle.add_constructor_to_transaction(sync_transaction,
        reinterpret_cast<void*>(&damage_session_fixture::construct),
        reinterpret_cast<void*>(&damage_session_fixture::destroy))==MH_OK);
    assert(sync_transaction.commit_pending_without_rollback()==MH_OK);
    auto* sync_controller=damage_session_fixture::state_object.data();
    assert(damage_session_fixture::construct(sync_controller)==sync_controller);
    put(sync_controller,0xDB0,reinterpret_cast<std::uintptr_t>(owner.data()));
    put(owner.data(),0x14F8,reinterpret_cast<std::uintptr_t>(sync_controller));
    StormboltPowerCellSyncToken sync_token{};
    assert(capture_stormbolt_power_cell_sync_controller(owner.data(),55,sync_store,sync_token));
    assert(mark_stormbolt_power_cell_sync_controller(sync_token,sync_store));
    StormboltPowerCellSyncEventState pending{};
    assert(sync_store.snapshot(sync_token,pending) && pending.pending);
    assert(complete_stormbolt_power_cell_sync_controller(sync_token,sync_store));
    assert(sync_store.snapshot(sync_token,pending) && !pending.pending);
    assert(damage_session_fixture::destroy(sync_controller,0)==sync_controller);
    assert(!mark_stormbolt_power_cell_sync_controller(sync_token,sync_store));
    assert(damage_session_fixture::construct(sync_controller)==sync_controller);
    assert(!complete_stormbolt_power_cell_sync_controller(sync_token,sync_store));
    StormboltPowerCellSyncToken rebuilt{};
    assert(sync_store.token_for_live_address(reinterpret_cast<std::uintptr_t>(sync_controller),rebuilt));
    assert(rebuilt.generation!=sync_token.generation && sync_store.snapshot(rebuilt,pending) && !pending.pending);
    StormboltPowerCellSyncFrames<2> producer_frames;
    StormboltPowerCellSyncSnapshot frame_snapshot{};frame_snapshot.controller_token=rebuilt;
    int outer_frame=0,inner_frame=0;
    const auto outer_stack=reinterpret_cast<std::uintptr_t>(&outer_frame);
    const auto inner_stack=reinterpret_cast<std::uintptr_t>(&inner_frame);
    const auto controller_address=reinterpret_cast<std::uintptr_t>(sync_controller);
    assert(producer_frames.begin(outer_stack,frame_snapshot) && producer_frames.begin(inner_stack,frame_snapshot));
    assert(!producer_frames.begin(outer_stack,frame_snapshot));
    assert(mark_stormbolt_power_cell_sync_controller(rebuilt,sync_store));
    assert(!producer_frames.complete(inner_stack,controller_address+1,sync_store) && producer_frames.active());
    assert(producer_frames.complete(inner_stack,controller_address,sync_store));
    assert(sync_store.snapshot(rebuilt,pending) && !pending.pending);
    assert(mark_stormbolt_power_cell_sync_controller(rebuilt,sync_store));
    assert(damage_session_fixture::destroy(sync_controller,0)==sync_controller);
    assert(damage_session_fixture::construct(sync_controller)==sync_controller);
    StormboltPowerCellSyncToken newest{};
    assert(sync_store.token_for_live_address(controller_address,newest));
    assert(mark_stormbolt_power_cell_sync_controller(newest,sync_store));
    assert(!producer_frames.complete(outer_stack,controller_address,sync_store) && !producer_frames.active());
    assert(sync_store.snapshot(newest,pending) && pending.pending);
    assert(damage_session_fixture::destroy(sync_controller,0)==sync_controller);
    assert(sync_transaction.stop()==MH_OK && sync_lifecycle.release_after_unhook());
    std::puts("PASS: shared eleven-hook transaction, damage/sync lifecycle, four CALL paths and activation rollback");
}
