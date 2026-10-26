#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_shoot_gatling_predicate.h"
#include "generated_shoot_gatling_cases.h"
#include "stormbolt_shoot_gatling_native_fields.h"
#include "stormbolt_shoot_gatling_condition.h"
#include "stormbolt_source_manager_attachment.h"
#include "stormbolt_source_manager_lifecycle.h"
#include "generated_source_command_fixture.h"
#include <Windows.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace stormslinger;
namespace {
void* native_storage=nullptr;
void* (*native_place)(void*,void*)=nullptr;
int (*destroy_probe)(void*)=nullptr;
bool fail_create=false,reset_ready=false,ready_value=true;
bool reset_query=false;
bool (*native_held)(void*,std::uint32_t)=nullptr;
unsigned destroy_calls=0;
unsigned held_calls=0;
unsigned manager_initializes=0,manager_rebuilds=0,manager_ticks=0,manager_destroys=0;
StormboltInstanceStore<1>* active_store=nullptr;
void* active_weapon=nullptr;
void* create_condition(void* type) {return fail_create?nullptr:native_place(type,native_storage);}
void* destroy_condition(void* object,std::uint32_t flags) {
    assert(flags==1);
    // Its sidecar owner is retired before calling the native destructor ABI.
    assert(destroy_probe && destroy_probe(object)==-1);++destroy_calls;return object;
}
bool ready_condition(void*,void* weapon,void*) {
    assert(weapon==active_weapon);
    if(reset_ready) active_store->initialize(reinterpret_cast<std::uintptr_t>(weapon));
    return ready_value;
}
bool held_condition(void* input,std::uint32_t key) {
    ++held_calls;
    if(reset_query) active_store->initialize(reinterpret_cast<std::uintptr_t>(active_weapon));
    return native_held(input,key);
}
__declspec(noinline) void manager_rebuild(void* manager) {
    assert(stormbolt_source_field<std::uint32_t>(manager,0xAD8)==798);
    ++manager_rebuilds;
}
__declspec(noinline) void manager_initialize(void* manager) {
    assert(stormbolt_source_field<std::uint32_t>(manager,0xAD8)==798);
    ++manager_initializes;manager_rebuild(manager); // Native nested init->rebuild.
}
__declspec(noinline) void manager_tick(void*) {++manager_ticks;}
__declspec(noinline) void* manager_destroy(void* manager,std::uint32_t flags) {
    assert(flags==5 && stormbolt_source_field<std::uint32_t>(manager,0xAD8)==798);
    ++manager_destroys;return manager;
}
}
int main() {
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,command_fixture_image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(image);
    for(const auto& span:command_fixture_spans) std::memcpy(image+span.rva,span.data,span.size);
    std::memcpy(image+0x3EEABF0,command_fixture_mapping,sizeof(command_fixture_mapping));
    for(const auto& span:command_fixture_spans) {
        DWORD old{};assert(VirtualProtect(image+span.rva,span.size,PAGE_EXECUTE_READ,&old));
    }
    assert(FlushInstructionCache(GetCurrentProcess(),image,command_fixture_image_size));
    std::array<unsigned char,0x28> transition{};
    std::array<unsigned char,0x2850> weapon{};
    std::array<unsigned char,0x12610> owner{};
    std::array<unsigned char,0x900> controller{};
    std::array<unsigned char,0x1800> parameter{};
    std::array<unsigned char,0x100> physical{};
    auto put=[](auto& object,std::size_t offset,auto value) {std::memcpy(object.data()+offset,&value,sizeof(value));};
    put(transition,0x20,controller.data()+0x10);put(weapon,0x9B0,owner.data());
    put(owner,0x12608,controller.data());put(controller,0x40,owner.data());
    put(owner,0xE480,physical.data());put(owner,0x7810,parameter.data());
    const auto parameter_vtable=reinterpret_cast<std::uintptr_t>(&parameter);
    put(parameter,0,parameter_vtable);
    StormboltShootGatlingNativeApi api{
        reinterpret_cast<bool(*)(void*,std::uint32_t)>(image+0x122FD90),
        reinterpret_cast<bool(*)(void*,std::uint32_t)>(image+0x1230070),parameter_vtable};
    auto set_actor_flag=reinterpret_cast<void(*)(void*,std::uint32_t)>(image+0x1F604B0);
    const float values[]={-1.0f,-0.0f,0.0f,1.0f,
        std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};
    for(const auto& test:shoot_gatling_cases) {
        StormboltShootGatlingInput input{};
        input.ammo=test.ammo;input.stage=test.stage;
        input.held=(test.bits&1)!=0;input.owner_blocked=(test.bits&2)!=0;
        input.owner_live=(test.bits&4)!=0;input.primary_fire=(test.bits&8)!=0;
        input.fallback_fire=(test.bits&16)!=0;input.resource=values[test.resource];
        input.action_timer=values[test.timer];input.stage_mode=static_cast<std::uint8_t>(test.mode);
        const auto saved=input;
        assert(stormbolt_evaluate_shoot_gatling(input)==test.result);
        assert(!std::memcmp(&input,&saved,sizeof(input)));
        put(weapon,0x27C8,test.ammo);put(weapon,0x2358,input.resource);
        put(controller,0x10+0x78C,std::uint32_t{input.held?0x1000000u:0u});
        owner[0x8965]=0;
        if(input.owner_blocked) set_actor_flag(owner.data(),0x16D);
        owner[0xC]=input.owner_live?2:0;
        physical[0xD]=input.primary_fire?0x20:0;physical[0x21]=input.fallback_fire?8:0;
        if(test.stage<=8) parameter[0x1624+40*test.stage]=input.stage_mode;
        StormboltStageState state{};state.counter=test.stage;state.action_timer=input.action_timer;
        int native_result=99;
        assert(stormbolt_read_shoot_gatling_condition(transition.data(),weapon.data(),state,api,native_result));
        assert(native_result==test.result);
    }
    // Stage8 returns before parameter lookup; ordinary stages require the typed row.
    put(weapon,0x27C8,std::uint32_t{40});put(weapon,0x2358,1.0f);
    put(controller,0x10+0x78C,std::uint32_t{0x1000000});
    owner[0xC]=0;owner[0x8965]=0;put(owner,0x7810,static_cast<void*>(nullptr));
    StormboltStageState state{};state.counter=8;int result=99;
    assert(stormbolt_read_shoot_gatling_condition(transition.data(),weapon.data(),state,api,result) && result==3);
    state.counter=0;result=99;
    assert(!stormbolt_read_shoot_gatling_condition(transition.data(),weapon.data(),state,api,result) && result==99);
    // Owned anonymous native base object: actual PC placement/context, private
    // vtable and generation-aware callback; allocation/free are explicit mocks.
    std::array<unsigned char,0x48> storage{};storage.fill(0xA5);
    native_storage=storage.data()+16;
    native_place=reinterpret_cast<void*(*)(void*,void*)>(image+0x11CB3E0);
    auto** base_table=reinterpret_cast<void**>(image+0x31C0AA0);
    for(unsigned n=0;n<9;++n) base_table[n]=reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x100+n));
    base_table[8]=image+0x11CB480;
    const std::array<void*,9> original_table={base_table[0],base_table[1],base_table[2],base_table[3],base_table[4],base_table[5],base_table[6],base_table[7],base_table[8]};
    const auto actor_table=reinterpret_cast<std::uintptr_t>(&owner);
    const auto weapon_table=reinterpret_cast<std::uintptr_t>(&weapon);
    put(owner,0,actor_table);owner[0xC]=2;put(owner,0x76B0,weapon.data());
    put(owner,0x14F8,controller.data());put(owner,0x7810,parameter.data());
    put(weapon,0,weapon_table);weapon[0xC]=2;put(weapon,0x9F8,std::uint32_t{13});put(weapon,0x9FC,std::uint32_t{239});
    put(weapon,0x2388,std::uint32_t{1});
    owner[0x8965]=0;physical[0xD]=0x20;parameter[0x1624]=0;
    StormboltInstanceStore<1> store;active_store=&store;active_weapon=weapon.data();
    auto token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));
    StormboltShootConditionBindings bindings{&store,&create_condition,
        reinterpret_cast<void(*)(void*,void*)>(image+0x11CB480),&destroy_condition,
        base_table,actor_table,weapon_table,&ready_condition,nullptr,api};
    native_held=api.held_query;bindings.fields.held_query=&held_condition;
    StormboltShootGatlingCondition<1> condition(store);
    fail_create=true;assert(!condition.create(owner.data(),bindings));fail_create=false;
    assert(condition.create(owner.data(),bindings));
    assert(condition.object()==native_storage);
    auto** private_table=stormbolt_source_field<void**>(condition.object(),0);
    assert(private_table!=base_table && private_table[8]==base_table[8]);
    assert(!std::memcmp(base_table,original_table.data(),sizeof(original_table)));
    auto callback=reinterpret_cast<int(*)(void*)>(private_table[6]);
    destroy_probe=callback;
    assert(callback(nullptr)==-1 && callback(reinterpret_cast<void*>(1))==-1);
    assert(callback(condition.object())==1);
    assert(store.transact(token,[](auto& s) noexcept {s.counter=8;return true;}));
    assert(callback(condition.object())==3);
    for(const auto count:{0u,1u,0u,2u}) {
        put(weapon,0x2388,count);
        const auto saved_weapon=weapon;
        const auto saved_owner=owner;
        const auto queried=held_calls;
        assert(callback(condition.object())==(count?3:-1));
        assert(held_calls==queried+(count?1u:0u));
        assert(weapon==saved_weapon && owner==saved_owner);
        assert(store.snapshot(token,state) && state.counter==8);
    }
    put(weapon,0x9FC,std::uint32_t{237});assert(callback(condition.object())==-1);
    put(weapon,0x9FC,std::uint32_t{238});assert(callback(condition.object())==3);
    ready_value=false;assert(callback(condition.object())==-1);ready_value=true;
    reset_ready=true;assert(callback(condition.object())==-1);reset_ready=false;
    assert(!store.snapshot(token,state));
    reset_query=true;assert(callback(condition.object())==-1);reset_query=false;
    condition.destroy_after_detach();assert(!condition.object() && destroy_calls==1);
    assert(condition.create(owner.data(),bindings));
    private_table=stormbolt_source_field<void**>(condition.object(),0);
    auto native_destroy=reinterpret_cast<void*(*)(void*,std::uint32_t)>(private_table[0]);
    auto* retired=condition.object();assert(native_destroy(retired,1)==retired);
    assert(!condition.object() && destroy_calls==2 && callback(retired)==-1);
    // Rebuild/destruction ownership order around the real manager field layout.
    alignas(8) std::array<unsigned char,0x1020> manager{};
    const auto manager_table=reinterpret_cast<std::uintptr_t>(&manager);
    put(manager,0,manager_table);put(manager,0xD90,owner.data());
    put(owner,0x12608,manager.data());put(manager,0x40,owner.data());
    std::array<StormboltGraphState,120> graph_states{};
    std::vector<std::vector<StormboltGraphLink>> graph_originals;
    graph_originals.reserve(std::size(source_graph_edits));
    for(const auto& edit:source_graph_edits) {
        graph_originals.emplace_back(edit.expected,edit.expected+edit.count);
        graph_states[edit.state].lists[edit.secondary]={graph_originals.back().data(),static_cast<std::uint32_t>(edit.count),0};
    }
    const auto graph_snapshot=graph_states;
    std::array<void*,798> registry{};registry[776]=&condition;
    put(manager,0xAB0,graph_states.data());put(manager,0xAB8,std::uint32_t{120});
    put(manager,0xAD0,registry.data());put(manager,0xAD8,std::uint32_t{798});
    StormboltSourceManagerBindings manager_bindings{manager_table,bindings};
    StormboltSourceManagerAttachment<1> attachment(store);
    assert(attachment.after_rebuild(manager.data(),manager_bindings)==StormboltManagerAttachResult::attached);
    auto* attached=attachment.condition();
    auto** attached_table=stormbolt_source_field<void**>(attached,0);
    assert(reinterpret_cast<int(*)(void*)>(attached_table[6])(attached)==1);
    const auto& first_edit=source_graph_edits[0];
    auto& first_list=graph_states[first_edit.state].lists[first_edit.secondary];
    auto* published=first_list.links;first_list.links=nullptr;
    const auto prior_destroys=destroy_calls;
    assert(!attachment.before_rebuild_or_destroy() && attachment.condition()==attached);
    assert(destroy_calls==prior_destroys && stormbolt_source_field<std::uint32_t>(manager.data(),0xAD8)==799);
    first_list.links=published;assert(attachment.before_rebuild_or_destroy());
    assert(destroy_calls==prior_destroys+1 && !attachment.active());
    assert(stormbolt_source_field<void**>(manager.data(),0xAD0)==registry.data());
    assert(stormbolt_source_field<std::uint32_t>(manager.data(),0xAD8)==798);
    assert(!std::memcmp(graph_states.data(),graph_snapshot.data(),sizeof(graph_states)));
    assert(attachment.after_rebuild(manager.data(),manager_bindings)==StormboltManagerAttachResult::attached);
    assert(attachment.before_rebuild_or_destroy());
    put(weapon,0x9FC,std::uint32_t{237});
    assert(attachment.after_rebuild(manager.data(),manager_bindings)==StormboltManagerAttachResult::unrelated);
    assert(!attachment.condition() && !attachment.active());
    put(weapon,0x9FC,std::uint32_t{239});
    StormboltInstanceToken current_token{};
    assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon.data()),current_token));
    assert(store.retire(current_token)); // Init precedes weapon-sidecar readiness.
    HookTransaction transaction;assert(transaction.begin()==MH_OK);
    StormboltSourceManagerLifecycle<1,1> lifecycle(store);
    const StormboltSourceManagerLifecycleBindings lifecycle_bindings{
        &manager_initialize,&manager_rebuild,&manager_tick,&manager_destroy};
    assert(lifecycle.prepare(transaction,lifecycle_bindings,manager_bindings)==MH_OK);
    assert(transaction.commit()==MH_OK);
    manager_initialize(manager.data());
    assert(manager_initializes==1 && manager_rebuilds==1 && !lifecycle.active_for(manager.data()));
    token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));
    manager_tick(manager.data());
    assert(manager_ticks==1 && lifecycle.active_for(manager.data()));
    manager_rebuild(manager.data());
    assert(manager_rebuilds==2 && lifecycle.active_for(manager.data()));
    const auto creates_before_reset=destroy_calls;
    store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));
    manager_tick(manager.data());
    assert(manager_ticks==2 && destroy_calls==creates_before_reset+1 && lifecycle.active_for(manager.data()));
    published=first_list.links;first_list.links=nullptr;
    manager_rebuild(manager.data());
    assert(manager_rebuilds==2 && lifecycle.blocked_native_calls==1);
    first_list.links=published;manager_rebuild(manager.data());
    assert(manager_rebuilds==3 && lifecycle.active_for(manager.data()));
    // Capacity exhaustion forwards an unrelated manager unchanged.
    auto second_manager=manager;
    put(second_manager,0xAD0,registry.data());put(second_manager,0xAD8,std::uint32_t{798});
    const auto destroys_before_overflow=destroy_calls;
    manager_tick(second_manager.data());
    assert(!lifecycle.active_for(second_manager.data()) && lifecycle.active_for(manager.data()));
    assert(destroy_calls==destroys_before_overflow);
    assert(manager_destroy(manager.data(),5)==manager.data());
    assert(manager_destroys==1 && !lifecycle.active_for(manager.data()));
    manager_initialize(manager.data());
    assert(lifecycle.active_for(manager.data()));
    assert(lifecycle.restore_all());
    manager_tick(manager.data()); // Hooks still present while draining: no reattach.
    assert(!lifecycle.active_for(manager.data()) && stormbolt_source_field<std::uint32_t>(manager.data(),0xAD8)==798);
    assert(transaction.stop()==MH_OK && lifecycle.release_after_stop());
    assert(!std::memcmp(graph_states.data(),graph_snapshot.data(),sizeof(graph_states)));
    for(unsigned n=0;n<16;++n) assert(storage[n]==0xA5 && storage[0x38+n]==0xA5);
    assert(VirtualFree(image,0,MEM_RELEASE));
    std::puts("PASS62720 actual PS44-instruction ShootGatling cases: all stages, input/owner gates, NaN/zero/infinities, every result and readonly snapshots.");
}
