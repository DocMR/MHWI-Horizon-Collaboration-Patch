#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_shooting_session.h"
#include "stormbolt_damage_session_test_fixture.h"
#include "stormbolt_power_cell_sync_codec.h"
#include <cassert>
#include <cstdio>
#include <string>

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

void* stormbolt_test_shoot(void*, void*, void*, const void*, void*, std::uint32_t) noexcept(false);
void stormbolt_test_coefficient_call(void*, std::uint32_t, void*);
extern char stormbolt_test_shoot_return, stormbolt_test_coefficient_return;
int stormbolt_test_registers_intact = 0;
unsigned stormbolt_timeline_fixture_calls=0;
__declspec(noinline) void stormbolt_timeline_fixture(void*,const void*,void*,float frame,bool reset) {
    assert(frame==3.5f && reset);++stormbolt_timeline_fixture_calls;
}
void stormbolt_status_test_call(void*,void*);
void stormbolt_loop_test_call(void*,void*);
extern unsigned char stormbolt_status_test_patch_site,stormbolt_loop_test_patch_site;
void* stormbolt_loop_test_actor=nullptr;
unsigned char stormbolt_loop_test_flag=0,stormbolt_loop_test_use_site=1;
void (*stormbolt_loop_test_entry)()=&stormbolt_loop_update_entry;
void* stormbolt_status_test_seen=nullptr;
std::uintptr_t stormbolt_status_test_manager=0xABCD,stormbolt_status_test_next_manager=0;
unsigned char stormbolt_status_test_flag=0;
}
namespace {
struct TestTransaction : HookTransaction {
    bool fail_commit = false, apply_before_failed_commit = false, fail_stop = false;
    void (*before_failed_commit)() = nullptr;
    unsigned stops = 0, commits = 0;
    MH_STATUS commit_pending_without_rollback() {
        ++commits;
        if (!fail_commit) return HookTransaction::commit_pending_without_rollback();
        if (apply_before_failed_commit) assert(HookTransaction::commit_pending_without_rollback() == MH_OK);
        if (before_failed_commit) before_failed_commit();
        return MH_ERROR_MEMORY_PROTECT;
    }
    MH_STATUS stop() {
        ++stops;
        if (fail_stop) { fail_stop = false; return MH_ERROR_MEMORY_PROTECT; }
        return HookTransaction::stop();
    }
};
TestTransaction g_transaction;
StormboltShootingSession<4, TestTransaction> g_session;
using RootFunction=int(*)();
RootFunction root_original=nullptr;
int root_replacement() {return root_original()+7;}
// Model the already committed quest engine while the real shooting session
// independently adopts and restores its borrowed transaction.
struct CommittedRootFixture {
    HookTransaction transaction;
    unsigned char* memory=nullptr;
    std::array<unsigned char,32> before{};
    void begin() {
        memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        assert(memory);std::memset(memory,0x90,4096);
        memory[0]=0xB8;memory[1]=11;std::memset(memory+2,0,3);memory[5]=0xC3;
        std::memcpy(before.data(),memory,before.size());
        assert(transaction.begin()==MH_OK);
        assert(transaction.add(memory,reinterpret_cast<void*>(&root_replacement),reinterpret_cast<void**>(&root_original))==MH_OK);
        assert(transaction.commit()==MH_OK && value()==18);
    }
    int value() const {return reinterpret_cast<RootFunction>(memory)();}
    ~CommittedRootFixture() {
        if(!memory) return;
        assert(!g_transaction.has_library_state());
        assert(transaction.has_library_state() && value()==18);
        assert(transaction.stop()==MH_OK && value()==11);
        assert(!std::memcmp(before.data(),memory,before.size()));
        assert(VirtualFree(memory,0,MEM_RELEASE));
        std::puts("PASS: committed root survives shooting restoration; final root stop restores original bytes");
    }
};
template<std::size_t N> struct Object { std::array<unsigned char, N> data{}; };
alignas(16) std::array<unsigned char,0x38> source_storage{};
std::array<unsigned char,0x40> source_type{};
std::array<void*,2> source_dti_table{};
std::array<void*,7> source_condition_table{};
unsigned source_command_calls=0,source_condition_original_calls=0;
std::int32_t source_condition_original(void*) {++source_condition_original_calls;return -7;}
std::int32_t source_condition_foreign(void*) {return 55;}
__declspec(noinline) void* source_original_create(void*) {return nullptr;}
__declspec(noinline) void* source_place(void*,void* storage) {
    if(storage) {
        std::memset(storage,0,0x28);
        auto* table=source_condition_table.data();
        std::memcpy(storage,&table,sizeof(table));
    }
    return storage;
}
__declspec(noinline) void* source_destroy(void* object,std::uint32_t flags) {
    StormboltInstanceToken token{};
    assert(flags==1 && g_session.source_transition());
    assert(!g_session.source_transition()->instances().token_for_live_address(
        reinterpret_cast<std::uintptr_t>(object),token));
    return object;
}
void* source_heap(void* native_type) {assert(native_type==source_type.data());return source_type.data();}
void* source_allocate(void* heap,std::size_t bytes,std::uint32_t alignment,std::uint32_t,
    std::uint32_t a,std::uint32_t b,const void*) {
    assert(heap==source_type.data() && bytes==0x38 && alignment==16 && !a && !b);
    return source_storage.data();
}
Object<0x2860> g_one, g_two;
Object<0xE478> g_actor;
unsigned evade_originals=0,evade_effects=0;
void* evade_expected_provider=&g_actor;
void evade_original(void* p) {assert(p==&g_one);++evade_originals;}
bool evade_ready(void* p,void* context) {return p==&g_one && context==&g_actor;}
void evade_request(void* provider,std::uint32_t event,std::uint32_t record,std::uint32_t argument) {
    assert(provider==evade_expected_provider && event==2000 && record==0 && argument==0xFFFFFFFF && evade_originals==evade_effects+1);
    ++evade_effects;
}
Object<0x30> g_action, g_event;
Object<0x176C> g_parameters;
Object<0xB8> g_list;
Object<0x6B0> g_info;
Object<0x126E0> pc_owner;
Object<0x1020> manager_object;
Object<0x5000> manager_input_root;
Object<0x84> manager_physical;
Object<0xA8> manager_component;
Object<0x668> manager_status;
Object<0x40> manager_authority;
Object<0x146F0> manager_system;
Object<0xAE48> manager_players;
void* manager_authority_slot=&manager_authority;
void* manager_system_slot=&manager_system;
void* manager_players_slot=&manager_players;
alignas(16) std::array<unsigned char,0x28> manager_condition_storage{};
std::array<void*,9> manager_base_table{};
std::array<StormboltGraphState,120> manager_rows{};
std::array<void*,798> manager_registry{};
std::vector<std::vector<StormboltGraphLink>> manager_original_links;
unsigned manager_rebuild_calls=0,manager_tick_calls=0,manager_condition_destroys=0;
__declspec(noinline) void session_manager_rebuild(void* manager) {
    assert(stormbolt_source_field<std::uint32_t>(manager,0xAD8)==798);++manager_rebuild_calls;
}
__declspec(noinline) void session_manager_initialize(void* manager) {session_manager_rebuild(manager);}
__declspec(noinline) void session_manager_tick(void*) {++manager_tick_calls;}
__declspec(noinline) void* session_manager_destroy(void* manager,std::uint32_t) {
    assert(stormbolt_source_field<std::uint32_t>(manager,0xAD8)==798);return manager;
}
void* manager_condition_create(void*) {
    manager_condition_storage.fill(0);auto* table=manager_base_table.data();
    std::memcpy(manager_condition_storage.data(),&table,sizeof(table));return manager_condition_storage.data();
}
void manager_condition_initialize(void* object,void* actor) {
    auto* input=stormbolt_source_field<void*>(actor,0x14F8);
    std::memcpy(static_cast<unsigned char*>(object)+0x10,&actor,sizeof(actor));
    std::memcpy(static_cast<unsigned char*>(object)+0x18,&input,sizeof(input));
    input=static_cast<unsigned char*>(input)+0x10;
    std::memcpy(static_cast<unsigned char*>(object)+0x20,&input,sizeof(input));
}
void* manager_condition_destroy(void* object,std::uint32_t flags) {
    assert(flags==1 && stormbolt_source_field<std::uint32_t>(manager_object.data.data(),0xAD8)==798);
    ++manager_condition_destroys;return object;
}
Object<0x15A8> pc_skills;
Object<0xA10> pc_status;
Object<0x9D0> pc_loop;
Object<0xA98> pc_resource;
std::uintptr_t pc_slot=reinterpret_cast<std::uintptr_t>(&pc_resource);
std::array<std::uintptr_t, 18> g_list_table{};
std::array<std::uintptr_t, 0x2C8/8> g_actor_table{};
std::array<std::uintptr_t, 2> g_info_table{}, g_parameter_table{};
struct Record { std::uint64_t unused = 0; void* descriptor = nullptr; };
std::array<Record, 98> g_records{};
std::array<std::uint64_t, 98> g_descriptors{};
std::uintptr_t* g_weapon_table = nullptr;
std::uintptr_t* g_action_table = nullptr;
void* g_last_descriptor = nullptr;
void* g_last_type = nullptr;
unsigned g_flags = 0, g_create_calls = 0, g_coefficient_calls = 0;
unsigned g_action_calls = 0, g_query_calls = 0;
int g_native_shell = 0, g_owner = 0, g_context = 0, g_quest_calls = 0;
int (*g_quest_original)(int) = nullptr;
template<class Obj, class T> void put(Obj& object, std::size_t offset, const T& value) {
    assert(offset + sizeof(value) <= object.data.size());
    std::memcpy(object.data.data() + offset, &value, sizeof(value));
}
void initialize_weapon(Object<0x2860>& object, std::uint32_t id) {
    object.data.fill(0);
    put(object, 0, g_weapon_table); put(object, 0x9F8, std::uint32_t{13});
    put(object, 0xC, std::uint8_t{2});
    put(object, 0x9FC, id); put(object, 0x9B0, &g_actor); put(object, 0x1D90, &g_list);
    put(object,0x2388,std::uint32_t{1}); // Dedicated shooting fixture has Power Source Mod.
}
__declspec(noinline) void pc_reset(void* p) {put(*static_cast<Object<0xA10>*>(p),0x20,std::uint32_t{0});}
__declspec(noinline) void* pc_construct(void* p,void* owner) {
    auto& o=*static_cast<Object<0xA10>*>(p);put(o,0,std::uintptr_t{2});put(o,8,owner);pc_reset(p);return p;
}
__declspec(noinline) void pc_destroy(void* p) {put(*static_cast<Object<0xA10>*>(p),0,std::uintptr_t{0});}
__declspec(noinline) void* pc_loop_construct(void* p) {
    auto& o=*static_cast<Object<0x9D0>*>(p);put(o,0,std::uintptr_t{4});put(o,0x60,static_cast<void*>(&pc_owner));return p;
}
__declspec(noinline) void* pc_loop_destroy(void* p,std::uint32_t flags) {
    assert(flags==5);put(*static_cast<Object<0x9D0>*>(p),0,std::uintptr_t{0});return p;
}
static unsigned skill_multiplier_calls=0;
__declspec(noinline) float pc_skill_multiplier(void*,float input,std::uint32_t) {
    ++skill_multiplier_calls;return input*2.0f;
}
__declspec(noinline) void* pc_sync_construct(void* p) {
    std::memset(p,0,0xDB8);*static_cast<std::uintptr_t*>(p)=77;return p;
}
__declspec(noinline) void* pc_sync_destroy(void* p,std::uint32_t flags) {
    assert(flags==0);*static_cast<std::uintptr_t*>(p)=0;return p;
}
__declspec(noinline) void* native_allocate(void* type) {
    g_last_type = type; initialize_weapon(g_one, 238); return &g_one;
}
__declspec(noinline) void* native_place(void* type, void* storage) {
    g_last_type = type;
    if (storage) initialize_weapon(*static_cast<Object<0x2860>*>(storage), storage == &g_one ? 238 : 239);
    return storage;
}
__declspec(noinline) void* native_destroy(void* object, std::uint32_t flags) {
    g_flags = flags;
    StormboltInstanceToken token{};
    assert(!g_session.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object), token));
    static_cast<Object<0x2860>*>(object)->data.fill(0);
    return object;
}
__declspec(noinline) const void* record_at(void* list, std::uint32_t index) {
    assert(list == &g_list);
    return index < g_records.size() ? &g_records[index] : nullptr;
}
__declspec(noinline) void coefficient_original(void*, std::uint32_t ammo, void* info) {
    assert(ammo == 40 && info == &g_info); ++g_coefficient_calls;
}
__declspec(noinline) void foreign_callback(void*, std::uint32_t, void*) { ++g_quest_calls; }
__declspec(noinline) bool action_original(void* action) { assert(action == &g_action); ++g_action_calls; return false; }
__declspec(noinline) bool foreign_action(void*) { ++g_action_calls; return true; }
void* action_event(void* actor) { assert(actor == &g_actor); return &g_event; }
bool action_query(void* actor, std::uint32_t channel) {
    assert(actor == &g_actor && channel == 0);
    std::uint32_t flags = 0;
    std::memcpy(&flags, g_event.data.data()+0x28, 4);
    assert(flags == 14); // Event bit0 cleared before query, all other bits preserved.
    void* weapon = nullptr;
    std::memcpy(&weapon, g_actor.data.data()+0x76B0, sizeof(weapon));
    StormboltInstanceToken token{};
    assert(g_session.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon), token));
    ++g_query_calls;
    return true;
}
__declspec(noinline) int quest_native(int input) { ++g_quest_calls; return input + 10; }
__declspec(noinline) int quest_hook(int input) { return g_quest_original(input) + 2; }
StormboltInstanceToken token_for(void* object) {
    StormboltInstanceToken token{};
    assert(g_session.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object), token));
    return token;
}
void fire(Object<0x2860>& object, std::uint32_t stage) {
    g_info.data.fill(0xA5);
    put(g_info, 0, g_info_table.data()); put(g_info, 0x80, &object);
    const auto creations = g_create_calls, callbacks = g_coefficient_calls;
    assert(stormbolt_test_shoot(&g_descriptors[40], &object, &g_owner, &g_context, &g_list, 40) == &g_native_shell);
    assert(stormbolt_test_registers_intact == 1 && g_create_calls == creations + 1);
    assert(g_last_descriptor == &g_descriptors[ps_stormbolt_selectors[stage]]);
    stormbolt_test_coefficient_call(&object, 40, &g_info);
    assert(g_coefficient_calls == callbacks + 1);
    float a = 0, b = 0;
    std::memcpy(&a, g_info.data.data() + 0x60, 4); std::memcpy(&b, g_info.data.data() + 0x6C, 4);
    assert(a == ps_stormbolt_rows[stage].fields[5] && b == ps_stormbolt_rows[stage].fields[6]);
}
void exchange_fixture_slot(std::uintptr_t value) {
    DWORD old = 0, unused = 0;
    assert(VirtualProtect(g_weapon_table, 4096, PAGE_READWRITE, &old));
    InterlockedExchangePointer(reinterpret_cast<void* volatile*>(g_weapon_table + 123), reinterpret_cast<void*>(value));
    assert(VirtualProtect(g_weapon_table, 4096, old, &unused));
}
void exchange_action_slot(std::uintptr_t value) {
    DWORD old = 0, unused = 0;
    assert(VirtualProtect(g_action_table, 4096, PAGE_READWRITE, &old));
    InterlockedExchangePointer(reinterpret_cast<void* volatile*>(g_action_table + 7), reinterpret_cast<void*>(value));
    assert(VirtualProtect(g_action_table, 4096, old, &unused));
}
std::array<std::uintptr_t, 9> g_special_table{};
std::array<std::uintptr_t, 34> g_provider_table{};
std::uintptr_t g_provider_object = 0, g_provider_dti = 0;
void* g_provider_slot = &g_provider_object;
Object<0x30> g_special_action;
Object<0x200> g_special_event;
Object<0x400> g_component;
Object<0x1AA0> g_bank;
unsigned g_special_motion = 0, g_special_create = 0, g_special_save = 0;
unsigned g_chain_fires=0;
void chain_fire(void* weapon,const void* origin,bool third,std::uint32_t fourth,bool fifth) {
    assert(weapon==&g_one && origin==&g_actor && third && fourth==0xFFFFFFFF && fifth);
    std::uint32_t ammo=0,bits=0;
    std::memcpy(&ammo,g_one.data.data()+0x27C8,4);
    std::memcpy(&bits,g_special_event.data.data()+0x190,4);
    assert(ammo==40 && !(bits&1)); // Action consumes before the native fire call.
    StormboltStageState state{};
    assert(g_session.instances().snapshot(token_for(&g_one),state));
    fire(g_one,state.shot_snapshot); // Real installed shell/coefficient hooks.
    ++g_chain_fires;
}
void boundary(void*) { assert(false); }
bool special_original(void*) { assert(false); return false; }
bool special_ready(void*, StormboltSpecialActionPhase) { return true; } // Fixture only.
bool special_motion(void*, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, float, float) { ++g_special_motion; return false; }
void special_flag(void*, std::uint32_t) { }
void* special_event(void*) { return &g_special_event; }
void special_weapon_effect(void*, std::uint32_t group, std::uint32_t, std::uint32_t) { assert(group == 0x7D0); }
bool special_query(void*, std::uint32_t) { return true; }
void* special_character(void* value) { return value; }
void special_save(void*, std::int32_t bit) { assert(bit == 11); ++g_special_save; }
void* special_construct(void*, void* value) { return value; }
void special_selector(void*, void*) { }
void* special_create(void* provider, void*, void*, std::uint32_t group, std::uint32_t,
    void*, void*, void*) { assert(provider == &g_provider_object && group == 0x7EE); ++g_special_create; return &g_context; }
std::uint8_t special_post_needed(void*) { return 0; }
void special_post(void*, void*, const void*, std::uint32_t) { assert(false); }
bool special_active(const void*) { return false; }
void* special_assign(void* bank, void* value) { assert(value == &g_context); return bank; }
void special_clear(void*) { }
StormboltSpecialNativeEndpoints special_endpoints() {
    g_special_table[6] = g_special_table[8] = reinterpret_cast<std::uintptr_t>(&boundary);
    g_special_table[7] = reinterpret_cast<std::uintptr_t>(&special_original);
    g_provider_table[4] = reinterpret_cast<std::uintptr_t>(&boundary);
    g_provider_table[31] = reinterpret_cast<std::uintptr_t>(&special_original);
    g_provider_table[33] = reinterpret_cast<std::uintptr_t>(&special_create);
    g_provider_object = reinterpret_cast<std::uintptr_t>(g_provider_table.data());
    g_provider_dti = reinterpret_cast<std::uintptr_t>(g_info_table.data());
    StormboltSpecialNativeEndpoints out;
    out.hook = {reinterpret_cast<std::uintptr_t>(g_special_table.data()),
        reinterpret_cast<std::uintptr_t>(g_actor_table.data()), reinterpret_cast<std::uintptr_t>(g_weapon_table),
        &boundary, &boundary, &special_original, &special_ready};
    out.action = {special_motion, special_flag, special_event, special_weapon_effect, special_query};
    out.save = {&g_provider_slot, special_character, special_save};
    out.effects = {&g_context, special_construct, special_selector, &g_provider_slot, &g_context,
        nullptr, special_post_needed, &g_provider_slot, special_post, special_active, special_assign, special_clear};
    out.provider_identity = {&g_provider_slot, g_provider_object, reinterpret_cast<std::uintptr_t>(&g_provider_dti),
        g_provider_dti, g_provider_table[4], g_provider_table[31], special_create};
    return out;
}
std::array<std::uintptr_t,9> g_start_table{};
std::array<std::uint32_t,41> g_start_items{};
Object<0x30> g_start_action;
unsigned g_start_original_calls=0;
void start_original_entry(void*) { ++g_start_original_calls; }
bool start_original_update(void*) { ++g_start_original_calls; return false; }
void* start_local_player(void*) { return &g_actor; }
void configure_start(StormboltNativeBindings& binding) {
    g_start_table[6]=reinterpret_cast<std::uintptr_t>(&start_original_entry);
    g_start_table[7]=reinterpret_cast<std::uintptr_t>(&start_original_update);
    g_start_table[8]=reinterpret_cast<std::uintptr_t>(&foreign_action);
    g_start_items[0]=137; g_start_items[33]=171;
    binding.gatling_entry.animation=binding.special_to_normal.action;
    binding.gatling_entry.player_manager_slot=&g_provider_slot;
    binding.gatling_entry.local_player=start_local_player;
    binding.gatling_entry.ammo_items=g_start_items.data();
    binding.gatling_hook={reinterpret_cast<std::uintptr_t>(g_start_table.data()),
        reinterpret_cast<std::uintptr_t>(g_actor_table.data()),reinterpret_cast<std::uintptr_t>(g_weapon_table),
        start_original_entry,nullptr,start_original_update,special_ready};
}
void start_transition() {
    put(g_start_action,0,g_start_table.data()); put(g_start_action,0x28,&g_actor);
    put(g_actor,0x76B0,&g_one); put(g_one,0x27DC,std::uint32_t{171});
    put(g_special_event,0x190,std::uint32_t{2});
    const auto motions=g_special_motion;
    reinterpret_cast<StormboltActionBoundary>(g_start_table[6])(&g_start_action);
    assert(reinterpret_cast<StormboltActionUpdate>(g_start_table[7])(&g_start_action));
    assert(g_special_motion==motions+3 && g_start_original_calls==0);
    std::uint32_t ammo=0; std::memcpy(&ammo,g_one.data.data()+0x27C8,4); assert(ammo==33);
    assert(g_start_table[8]==reinterpret_cast<std::uintptr_t>(&foreign_action));
}
void full_transition() {
    put(g_special_action, 0, g_special_table.data()); put(g_special_action, 0x28, &g_actor);
    put(g_actor, 0x76B0, &g_one); put(g_actor, 0x8808, &g_component); put(g_actor, 0x8938, std::uint8_t{1});
    put(g_component, 8, &g_bank); put(g_component, 0x18, &g_actor);
    put(g_special_event, 0x190, std::uint32_t{2}); put(g_special_event, 0x178, std::uint32_t{0x10000000});
    reinterpret_cast<StormboltActionBoundary>(g_special_table[6])(&g_special_action);
    assert(reinterpret_cast<StormboltActionUpdate>(g_special_table[7])(&g_special_action));
    reinterpret_cast<StormboltActionBoundary>(g_special_table[8])(&g_special_action);
    assert(g_special_motion == 3 && g_special_create == 1 && g_special_save == 1);
    std::uint32_t ammo = 0; std::memcpy(&ammo, g_one.data.data()+0x27C8, 4); assert(ammo == 40);
    std::uint32_t event_flags = 1; std::memcpy(&event_flags, g_special_event.data.data()+0x190, 4); assert(event_flags == 0);
}

void transition(Object<0x2860>& weapon) {
    put(g_actor, 0x76B0, &weapon);
    put(g_event, 0x28, std::uint32_t{15});
    const auto before = g_query_calls, originals = g_action_calls;
    assert(reinterpret_cast<StormboltActionUpdate>(g_action_table[7])(&g_action));
    assert(g_query_calls == before+1 && g_action_calls == originals);
}
}
extern "C" __declspec(noinline) void* stormbolt_test_create(void* descriptor, void* receiver, void* owner, const void* context) {
    ++g_create_calls; g_last_descriptor = descriptor;
    assert(receiver == &g_one || receiver == &g_two);
    assert(owner == &g_owner && context == &g_context);
    return &g_native_shell;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    std::string mode = argv[1];
    const bool coordinated=mode.rfind("coordinated-",0)==0;
    if(coordinated) mode.erase(0,12);
    const bool borrowed=mode.rfind("borrowed-",0)==0;
    if(borrowed) mode.erase(0,9);
    CommittedRootFixture root;
    assert(mode.rfind("motion-events",0)==0 || mode.rfind("hud-",0)==0 || mode.rfind("evade-",0)==0 || mode == "success" || mode == "cancel" || mode == "add-failure" || mode == "slot-failure" ||
        mode == "commit-failure" || mode == "commit-restore-conflict" || mode == "restore-conflict" || mode == "stop-failure" ||
        mode == "action-slot-failure" || mode == "action-restore-conflict" || mode == "commit-action-restore-conflict" ||
        mode == "full" || mode == "full-restore" || mode == "full-cancel" ||
        mode == "full-slot-failure" || mode == "full-commit-failure" || mode == "full-commit-restore" ||
        mode == "full-start" || mode == "full-start-restore" || mode == "full-start-slot-failure" || mode == "full-middle" || mode == "full-middle-restore" ||
        mode == "full-middle-slot-failure" || mode == "full-middle-cancel" || mode == "full-middle-chain" || mode == "full-last" || mode == "full-last-restore" || mode == "full-last-slot-failure" || mode == "full-last-cancel" || mode == "full-update" || mode == "full-update-restore" || mode == "full-update-slot-failure" || mode == "full-update-cancel" || mode.rfind("full-update-power-cell",0)==0 || mode.rfind("source-",0)==0);
    const bool power_cell=mode.rfind("full-update-power-cell",0)==0;
    const bool update=mode.rfind("full-update",0)==0;
    const bool last=mode.rfind("full-last",0)==0 || update;
    const bool middle = mode.rfind("full-middle",0)==0 || last;
    const bool start = mode.rfind("full-start",0)==0 || middle;
    const bool full = mode.rfind("full", 0) == 0;
    g_weapon_table = static_cast<std::uintptr_t*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(g_weapon_table);
    g_weapon_table[0x228/8] = reinterpret_cast<std::uintptr_t>(&start_original_entry);
    g_weapon_table[123] = reinterpret_cast<std::uintptr_t>(&coefficient_original);
    g_action_table = static_cast<std::uintptr_t*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(g_action_table);
    g_action_table[7] = reinterpret_cast<std::uintptr_t>(&action_original);
    DWORD old = 0;
    assert(VirtualProtect(g_weapon_table, 4096, PAGE_READONLY, &old));
    assert(VirtualProtect(g_action_table, 4096, PAGE_READONLY, &old));
    g_list_table[17] = reinterpret_cast<std::uintptr_t>(&record_at);
    put(g_list, 0, g_list_table.data()); put(g_list, 0xA8, g_records.data()); put(g_list, 0xB0, std::uint32_t{98});
    put(g_actor, 0xC, std::uint8_t{2}); put(g_actor, 0x7810, &g_parameters);
    put(g_actor, 0, g_actor_table.data()); put(g_actor, 0xE470, &g_event);
    put(g_action, 0, g_action_table); put(g_action, 0x28, &g_actor);
    g_actor_table[0x2C0/8] = reinterpret_cast<std::uintptr_t>(&action_query);
    put(g_parameters, 0, g_parameter_table.data());
    for (unsigned i = 0; i < 98; ++i) g_records[i].descriptor = &g_descriptors[i];
    for (unsigned i = 0; i < 9; ++i)
        std::memcpy(g_parameters.data.data() + 0x1608 + 40 * i, ps_stormbolt_rows[i].fields.data(), 28);
    StormboltNativeBindings binding{};
    binding.lifecycle = {reinterpret_cast<void*>(&native_allocate), reinterpret_cast<void*>(&native_place), reinterpret_cast<void*>(&native_destroy)};
    binding.shell = {reinterpret_cast<void*>(&stormbolt_test_create), reinterpret_cast<std::uintptr_t>(&stormbolt_test_shoot_return),
        reinterpret_cast<std::uintptr_t>(g_weapon_table), reinterpret_cast<std::uintptr_t>(g_list_table.data()), &record_at};
    binding.coefficients = {g_weapon_table + 123, &coefficient_original,
        reinterpret_cast<std::uintptr_t>(&stormbolt_test_coefficient_return), reinterpret_cast<std::uintptr_t>(g_weapon_table),
        reinterpret_cast<std::uintptr_t>(g_info_table.data()), reinterpret_cast<std::uintptr_t>(g_parameter_table.data())};
    binding.normal_to_special = {g_action_table + 7, &action_original,
        reinterpret_cast<std::uintptr_t>(g_action_table), reinterpret_cast<std::uintptr_t>(g_actor_table.data()),
        reinterpret_cast<std::uintptr_t>(g_weapon_table), &action_event, &action_query};
    if (full) binding.special_to_normal = special_endpoints();
    if (start) configure_start(binding);
    std::array<std::uintptr_t,9> middle_table{};
    middle_table[6]=reinterpret_cast<std::uintptr_t>(&start_original_entry);
    middle_table[7]=reinterpret_cast<std::uintptr_t>(&start_original_update);
    middle_table[8]=0x12345678;
    auto middle_hook=binding.gatling_hook;
    middle_hook.action_vtable=reinterpret_cast<std::uintptr_t>(middle_table.data());
    StormboltMiddleEntryApi middle_entry{
        +[](void*,std::uint32_t){},+[](void*){return false;},+[](void*,float,std::uint32_t){return true;},
        +[](void*)->std::uint8_t{return 0;},+[](void*,std::uint8_t){}};
    auto primary_table=middle_table;
    if(middle) {
        binding.primary_shoot_hook=middle_hook;
        binding.primary_shoot_hook.action_vtable=reinterpret_cast<std::uintptr_t>(primary_table.data());
        binding.middle_prepare={binding.special_to_normal.action,+[](void*){return 0.0f;},+[](void*,std::uint32_t,float){}};
        binding.middle_fire={action_event,+[](void* p)->const void*{return p;},+[](void*,const void*,bool,std::uint32_t,bool){}};
        if(mode=="full-middle-chain") binding.middle_fire={special_event,
            +[](void* p)->const void*{return p;},chain_fire};
    }
    auto chain_shot=[&](unsigned i) {
    Object<0x40> action;
    auto& table=i==0?primary_table:middle_table;
    put(action,0,table.data());put(action,0x28,&g_actor);
    put(g_special_event,0x190,std::uint32_t{0});
    reinterpret_cast<StormboltActionBoundary>(table[6])(&action);
    assert(!reinterpret_cast<StormboltActionUpdate>(table[7])(&action));
    assert(g_chain_fires==i); // Preparation cannot fire.
    // Actual slot244 key value, separately verified by the
    // retained native scheduler. This fixture injects the
    // event; it does not run game input or the scheduler.
    put(g_special_event,0x190,std::uint32_t{0x401});
    assert(reinterpret_cast<StormboltActionUpdate>(table[7])(&action));
    assert(g_chain_fires==i+1);
    assert(reinterpret_cast<StormboltActionUpdate>(table[7])(&action));
    assert(g_chain_fires==i+1); // Consumed event cannot repeat.
    StormboltStageState state{};assert(g_session.instances().snapshot(token_for(&g_one),state));
    assert(state.counter==i+1 && state.shot_snapshot==i);
    };
    std::array<std::uintptr_t,9> last_table{};
    last_table[6]=last_table[8]=reinterpret_cast<std::uintptr_t>(&start_original_entry);
    last_table[7]=reinterpret_cast<std::uintptr_t>(&start_original_update);
    const auto last_original=last_table;
    if(last) {
        binding.middle_entry=middle_entry; binding.middle_hook=middle_hook;
        binding.last_hook=middle_hook;
        binding.last_hook.action_vtable=reinterpret_cast<std::uintptr_t>(last_table.data());
        binding.last_hook.original_exit=&start_original_entry;
        binding.last_entry={middle_entry.mode,middle_entry.parameter,+[](void*,std::uint32_t,std::uint32_t){}};
        binding.last_update={binding.middle_fire.event_object,binding.middle_prepare.difference,
            binding.middle_fire.origin,binding.middle_fire.fire,binding.special_to_normal.action.actor_flag,
            nullptr,+[](void*){return false;}};
    }
    if(update) {
        binding.weapon_update={reinterpret_cast<std::uintptr_t>(g_weapon_table),&start_original_entry,+[](void*){return false;}};
        auto& api=binding.stage_update;
        api.gate={binding.gatling_entry.player_manager_slot,+[](void*,std::uint32_t)->std::uint8_t{return 0;}};
        api.descriptor=+[](void*,std::uint32_t)->std::uint64_t{return 0;};
        api.ammo_items=binding.gatling_entry.ammo_items;
        auto& r=api.routes;
        r.input={+[](void*,std::uint32_t){return false;}};
        r.trigger={r.input.query,r.input.query};
        r.attachment=+[](void*)->std::uint32_t{return 0;};
        r.effect={+[](const void*){return false;},+[](void*,std::uint32_t,std::uint32_t,std::uint32_t)->void*{return nullptr;},+[](void* p,void*)->void*{return p;}};
        r.cleanup={r.effect.active,+[](void*){},r.effect.assign};
        r.owner={binding.gatling_entry.player_manager_slot,binding.gatling_entry.local_player};
        r.actor_flag=binding.special_to_normal.action.actor_flag;
        r.color={r.effect.active,+[](void*,const std::uint32_t*){}};
        r.material={+[](void*,std::uint32_t,const float*){}};
    }
    const std::array<void*, 5> targets{binding.lifecycle.allocate, binding.lifecycle.place, binding.lifecycle.destroy,
        binding.shell.create_entry, reinterpret_cast<void*>(&quest_native)};
    std::array<std::array<unsigned char, 32>, 5> original{};
    for (std::size_t i = 0; i < targets.size(); ++i) std::memcpy(original[i].data(), targets[i], 32);
    auto verify_restored = [&] {
        assert(g_weapon_table[0x228/8]==reinterpret_cast<std::uintptr_t>(&start_original_entry));
        if(last) assert(last_table==last_original);
        if(middle) {
            assert(middle_table[6]==reinterpret_cast<std::uintptr_t>(&start_original_entry));
            assert(middle_table[7]==reinterpret_cast<std::uintptr_t>(&start_original_update));
            assert(middle_table[8]==0x12345678);
        }
        if (start) {
            assert(g_start_table[6]==reinterpret_cast<std::uintptr_t>(&start_original_entry));
            assert(g_start_table[7]==reinterpret_cast<std::uintptr_t>(&start_original_update));
            assert(g_start_table[8]==reinterpret_cast<std::uintptr_t>(&foreign_action));
        }
        for (std::size_t i = 0; i < targets.size(); ++i) assert(!std::memcmp(original[i].data(), targets[i], 32));
        assert(g_weapon_table[123] == reinterpret_cast<std::uintptr_t>(&coefficient_original));
        assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&action_original));
        MEMORY_BASIC_INFORMATION memory{};
        assert(VirtualQuery(g_action_table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
        assert(VirtualQuery(g_weapon_table, &memory, sizeof(memory)) && memory.Protect == PAGE_READONLY);
        if (full) {
            assert(g_special_table[6] == reinterpret_cast<std::uintptr_t>(&boundary));
            assert(g_special_table[7] == reinterpret_cast<std::uintptr_t>(&special_original));
            assert(g_special_table[8] == reinterpret_cast<std::uintptr_t>(&boundary));
        }
        assert(!g_transaction.has_library_state());
        assert(quest_native(1) == 11);
        assert(g_session.restore().ok());
    };
    assert(!g_session.activate().ok());
    if(borrowed) {
        root.begin();
        assert(g_transaction.begin_shared_with(root.transaction)==MH_OK);
    } else assert(g_transaction.begin() == MH_OK);
    assert(g_transaction.add(targets[4], reinterpret_cast<void*>(&quest_hook), reinterpret_cast<void**>(&g_quest_original)) == MH_OK);
    auto invalid = binding; invalid.coefficients.weapon_vtable = 0;
    assert(g_session.prepare(g_transaction, invalid).issue == StormboltSessionIssue::invalid_bindings);
    invalid = binding; invalid.normal_to_special.weapon_vtable = 0;
    assert(g_session.prepare(g_transaction, invalid).issue == StormboltSessionIssue::invalid_bindings);
    invalid = binding; invalid.normal_to_special.slot = g_action_table+6;
    assert(g_session.prepare(g_transaction, invalid).issue == StormboltSessionIssue::invalid_bindings);
    assert(g_session.state() == StormboltSessionState::fresh && g_transaction.can_add_hooks() && g_transaction.last.owned == 1);
    if (mode == "add-failure") binding.lifecycle.place = reinterpret_cast<void*>(1);
    if (mode == "slot-failure") binding.coefficients.original = &foreign_callback;
    if (mode == "action-slot-failure") binding.normal_to_special.original = &foreign_action;
    if (full) {
        auto incomplete = binding; incomplete.special_to_normal.hook.ready = nullptr;
        assert(g_session.prepare_full(g_transaction, incomplete).issue == StormboltSessionIssue::invalid_bindings);
        assert(g_transaction.last.owned == 1 && g_session.state() == StormboltSessionState::fresh);
        incomplete = binding; incomplete.special_to_normal.hook.action_vtable = reinterpret_cast<std::uintptr_t>(g_weapon_table)+0x3A8;
        assert(g_session.prepare_full(g_transaction, incomplete).issue == StormboltSessionIssue::invalid_bindings);
        if (mode == "full-slot-failure") binding.special_to_normal.hook.original_update = &foreign_action;
    }
    if (start) {
        auto missing=binding; missing.gatling_hook.ready=nullptr;
        assert(g_session.prepare_with_start(g_transaction,missing).issue==StormboltSessionIssue::invalid_bindings);
        assert(g_transaction.last.owned==1);
        if (mode=="full-start-slot-failure") binding.gatling_hook.original_update=&foreign_action;
    }
    if(middle) {
        auto missing=middle_entry; missing.parameter=nullptr;
        assert(g_session.prepare_with_middle(g_transaction,binding,missing,middle_hook).issue==StormboltSessionIssue::invalid_bindings);
        assert(g_session.state()==StormboltSessionState::fresh);
        for(const auto table : {binding.gatling_hook.action_vtable,binding.special_to_normal.hook.action_vtable,
            binding.normal_to_special.action_vtable,reinterpret_cast<std::uintptr_t>(binding.coefficients.slot)-0x30,
            (std::numeric_limits<std::uintptr_t>::max)()-7}) {
            auto overlap=middle_hook; overlap.action_vtable=table;
            assert(g_session.prepare_with_middle(g_transaction,binding,middle_entry,overlap).issue==StormboltSessionIssue::invalid_bindings);
            assert(g_session.state()==StormboltSessionState::fresh && g_transaction.last.owned==1);
        }
        if(mode=="full-middle-slot-failure") middle_hook.original_update=&foreign_action;
    }
    if(last) {
        auto missing=binding; missing.last_update.complete=nullptr;
        assert(g_session.prepare_with_last(g_transaction,missing).issue==StormboltSessionIssue::invalid_bindings);
        assert(g_session.state()==StormboltSessionState::fresh && g_transaction.last.owned==1);
        for(const auto table : {binding.middle_hook.action_vtable,binding.gatling_hook.action_vtable,
            binding.special_to_normal.hook.action_vtable,binding.normal_to_special.action_vtable,
            reinterpret_cast<std::uintptr_t>(binding.coefficients.slot)-0x40,
            (std::numeric_limits<std::uintptr_t>::max)()-7}) {
            auto overlap=binding; overlap.last_hook.action_vtable=table;
            assert(g_session.prepare_with_last(g_transaction,overlap).issue==StormboltSessionIssue::invalid_bindings);
            assert(g_session.state()==StormboltSessionState::fresh && g_transaction.last.owned==1);
        }
        if(mode=="full-last-slot-failure") binding.last_hook.original_update=&foreign_action;
    }
    if(update) {
        auto missing=binding; missing.weapon_update.ready=nullptr;
        assert(g_session.prepare_with_update(g_transaction,missing,0,{},{}).issue==StormboltSessionIssue::invalid_bindings);
        missing=binding; missing.stage_update.routes.material.write_vector=nullptr;
        assert(g_session.prepare_with_update(g_transaction,missing,0,{},{}).issue==StormboltSessionIssue::invalid_bindings);
        missing=binding; missing.last_hook.action_vtable=binding.weapon_update.weapon_vtable+0x228-0x30;
        assert(g_session.prepare_with_update(g_transaction,missing,0,{},{}).issue==StormboltSessionIssue::invalid_bindings);
        assert(g_session.state()==StormboltSessionState::fresh && g_transaction.last.owned==1);
        if(mode=="full-update-slot-failure") binding.weapon_update.original=+[](void*){};
    }
    if(coordinated) assert(g_session.use_shared_transaction_coordinator());
    auto prepared = update ? g_session.prepare_with_update(g_transaction,binding,0,{},{}) : last ? g_session.prepare_with_last(g_transaction,binding) : middle ? g_session.prepare_with_middle(g_transaction,binding,middle_entry,middle_hook) : start ? g_session.prepare_with_start(g_transaction,binding) : full ? g_session.prepare_full(g_transaction, binding) : g_session.prepare(g_transaction, binding);
    if(mode.rfind("motion-events",0)==0) {
        assert(prepared.ok());
        assert(g_session.attach_motion_events({&stormbolt_timeline_fixture,
            binding.normal_to_special.actor_vtable,binding.shell.weapon_vtable,nullptr,nullptr}).ok());
        auto restore_events=[&] {
            if(!coordinated) return g_session.restore();
            const auto restored=g_session.restore_before_shared_stop_while_quiescent();
            if(!restored.ok()) return restored;
            assert(g_transaction.stop()==MH_OK);
            return g_session.release_after_shared_stop_while_quiescent();
        };
        if(mode!="motion-events-cancel") {
            if(mode=="motion-events-commit-failure") g_transaction.fail_commit=true;
            if(coordinated) {
                const auto committed=g_transaction.commit_pending_without_rollback();
                if(mode=="motion-events-commit-failure") {
                    assert(committed!=MH_OK);assert(restore_events().ok());
                } else {
                    assert(committed==MH_OK);
                    assert(g_session.activate_after_shared_commit_while_quiescent().ok());
                }
            } else assert(g_session.activate().ok()==(mode!="motion-events-commit-failure"));
        }
        StormboltTimelineBind volatile invoke=&stormbolt_timeline_fixture;
        invoke(nullptr,nullptr,nullptr,3.5f,true);
        assert(stormbolt_timeline_fixture_calls==1);
        if(g_session.state()!=StormboltSessionState::stopped) assert(restore_events().ok());
        invoke(nullptr,nullptr,nullptr,3.5f,true);
        assert(stormbolt_timeline_fixture_calls==2);
        verify_restored();
        assert(VirtualFree(g_weapon_table,0,MEM_RELEASE));
        assert(VirtualFree(g_action_table,0,MEM_RELEASE));
        std::printf("PASS:motion events session %s coordinated=%u\n",mode.c_str(),unsigned(coordinated));
        return 0;
    }
    if(mode.rfind("hud-",0)==0) {
        assert(prepared.ok());
        alignas(8) std::array<void*,13> hud_table{};
        alignas(8) std::array<void*,9> hint_table{};
        alignas(8) std::array<void*,9> training_table{};
        alignas(8) std::array<void*,77> itemwindow_table{};
        alignas(8) std::array<void*,77> hint_update_table{},training_update_table{};
        auto hint_original=+[](void*,void*){};
        auto hint_foreign=+[](void*,void*){++g_special_create;};
        hint_table[8]=reinterpret_cast<void*>(hint_original);
        training_table[8]=reinterpret_cast<void*>(hint_original);
        auto hud_original=+[](void*){};
        auto foreign=+[](void*){++g_special_create;};
        hud_table[12]=reinterpret_cast<void*>(hud_original);
        itemwindow_table[76]=reinterpret_cast<void*>(hud_original);
        hint_update_table[76]=reinterpret_cast<void*>(hud_original);
        training_update_table[76]=reinterpret_cast<void*>(hud_original);
        StormboltChargeHudBindings hud{};
        hud.slot=&hud_table[12];hud.controller_vtable=reinterpret_cast<std::uintptr_t>(hud_table.data());
        hud.original=mode=="hud-install-failure"?foreign:hud_original;
        hud.input=+[](void*,StormboltChargeHudInput&){return false;};
        hud.api={+[](void*,bool){},+[](void*,float,std::uint8_t){},+[](void*){return 0.0f;},
            +[](void*){return std::uint32_t{1};},+[](void*,std::uint32_t,void*){return true;}};
        auto restore_hud=[&] {
            if(!coordinated) return g_session.restore();
            const auto restored=g_session.restore_before_shared_stop_while_quiescent();
            if(!restored.ok()) return restored;
            assert(g_transaction.stop()==MH_OK);
            return g_session.release_after_shared_stop_while_quiescent();
        };
        const auto attached=g_session.attach_charge_hud(hud);
        if(mode=="hud-install-failure") {
            assert(!attached.ok() && attached.issue==StormboltSessionIssue::hud_prepare);
            if(coordinated) assert(restore_hud().ok());
        } else {
            assert(attached.ok());
            assert(g_session.attach_operation_hint({reinterpret_cast<std::uintptr_t>(hint_table.data()),
                hint_original,+[](bool&){return false;},reinterpret_cast<std::uintptr_t>(hint_update_table.data()),
                hud_original}).ok());
            assert(g_session.attach_training_hint({reinterpret_cast<std::uintptr_t>(training_table.data()),
                hint_original,+[](bool&){return false;},reinterpret_cast<std::uintptr_t>(training_update_table.data()),
                hud_original}).ok());
            assert(g_session.attach_itemwindow({reinterpret_cast<std::uintptr_t>(itemwindow_table.data()),
                hud_original,+[](void*,StormboltChargeHudInput&){return false;},hud.api}).ok());
            const auto itemwindow_owned=itemwindow_table[76];
            const auto training_owned=training_table[8];
            const auto hint_update_owned=hint_update_table[76];
            const auto training_update_owned=training_update_table[76];
            const auto hint_owned=hint_table[8];
            const auto owned=hud_table[12];assert(owned!=reinterpret_cast<void*>(hud_original));
            if(mode!="hud-cancel") {
                if(mode=="hud-commit-failure") g_transaction.fail_commit=true;
                if(coordinated) {
                    const auto committed=g_transaction.commit_pending_without_rollback();
                    if(mode=="hud-commit-failure") {
                        assert(committed!=MH_OK);assert(restore_hud().ok());
                    } else {
                        assert(committed==MH_OK);
                        assert(g_session.activate_after_shared_commit_while_quiescent().ok());
                    }
                } else {
                    const auto activated=g_session.activate();
                    assert(activated.ok()==(mode!="hud-commit-failure"));
                }
            }
            if(mode=="hud-restore-conflict") {
                const auto stops=g_transaction.stops;
                hud_table[12]=reinterpret_cast<void*>(foreign);
                assert(restore_hud().issue==StormboltSessionIssue::hud_restore);
                assert(g_transaction.stops==stops && g_transaction.has_library_state());
                assert(hud_table[12]==reinterpret_cast<void*>(foreign));
                hud_table[12]=owned;
            }
            if(mode=="hud-operation-restore-conflict") {
                const auto stops=g_transaction.stops;
                hint_table[8]=reinterpret_cast<void*>(hint_foreign);
                assert(restore_hud().issue==StormboltSessionIssue::hud_restore);
                assert(g_transaction.stops==stops && g_transaction.has_library_state());
                assert(hint_table[8]==reinterpret_cast<void*>(hint_foreign) && hud_table[12]==owned);
                hint_table[8]=hint_owned;
            }
            if(mode=="hud-itemwindow-restore-conflict") {
                const auto stops=g_transaction.stops;
                itemwindow_table[76]=reinterpret_cast<void*>(foreign);
                assert(restore_hud().issue==StormboltSessionIssue::hud_restore);
                assert(g_transaction.stops==stops && g_transaction.has_library_state());
                assert(itemwindow_table[76]==reinterpret_cast<void*>(foreign) &&
                    training_table[8]==training_owned && hint_table[8]==hint_owned && hud_table[12]==owned);
                itemwindow_table[76]=itemwindow_owned;
            }
            if(mode=="hud-operation-update-restore-conflict" || mode=="hud-training-update-restore-conflict") {
                const bool training=mode=="hud-training-update-restore-conflict";
                auto& table=training?training_update_table:hint_update_table;
                const auto stops=g_transaction.stops;
                table[76]=reinterpret_cast<void*>(foreign);
                assert(restore_hud().issue==StormboltSessionIssue::hud_restore);
                assert(g_transaction.stops==stops && g_transaction.has_library_state());
                assert(table[76]==reinterpret_cast<void*>(foreign) && hud_table[12]==owned);
                assert((training?training_table:hint_table)[8]==(training?training_owned:hint_owned));
                table[76]=training?training_update_owned:hint_update_owned;
            }
            if(mode=="hud-training-restore-conflict") {
                const auto stops=g_transaction.stops;
                training_table[8]=reinterpret_cast<void*>(hint_foreign);
                assert(restore_hud().issue==StormboltSessionIssue::hud_restore);
                assert(g_transaction.stops==stops && g_transaction.has_library_state());
                assert(training_table[8]==reinterpret_cast<void*>(hint_foreign) &&
                    hint_table[8]==hint_owned && hud_table[12]==owned);
                training_table[8]=training_owned;
            }
            if(g_session.state()!=StormboltSessionState::stopped) assert(restore_hud().ok());
        }
        assert(hud_table[12]==reinterpret_cast<void*>(hud_original));
        assert(hint_table[8]==reinterpret_cast<void*>(hint_original));
        assert(training_table[8]==reinterpret_cast<void*>(hint_original));
        assert(itemwindow_table[76]==reinterpret_cast<void*>(hud_original));
        assert(hint_update_table[76]==reinterpret_cast<void*>(hud_original));
        assert(training_update_table[76]==reinterpret_cast<void*>(hud_original));
        verify_restored();
        assert(VirtualFree(g_weapon_table,0,MEM_RELEASE));
        assert(VirtualFree(g_action_table,0,MEM_RELEASE));
        std::printf("PASS:HUD shared session %s coordinated=%u\n",mode.c_str(),unsigned(coordinated));
        return 0;
    }
    if(mode.rfind("evade-",0)==0) {
        assert(prepared.ok());
        const auto restore_evade=[&]() {
            if(!coordinated) return g_session.restore();
            auto result=g_session.restore_before_shared_stop_while_quiescent();
            if(!result.ok()) return result;
            const auto stopped=g_transaction.stop();
            if(stopped!=MH_OK) return StormboltSessionResult{g_session.state(),StormboltSessionIssue::hook_restore,stopped};
            return g_session.release_after_shared_stop_while_quiescent();
        };
        const bool typed_evade=mode.rfind("evade-native-",0)==0;
        const std::string evade_mode=typed_evade ? "evade-"+mode.substr(13) : mode;
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(page);
        const unsigned char prefix[]{0x48,0x83,0xEC,0x28},suffix[]{0x48,0x83,0xC4,0x28,0xC3};
        std::memcpy(page,prefix,4);std::memcpy(page+9,suffix,5);
        page[0x100]=0x48;page[0x101]=0xB8;auto fn=&evade_original;std::memcpy(page+0x102,&fn,8);
        page[0x10A]=0xFF;page[0x10B]=0xE0;
        std::array<unsigned char,5> expected{};
        assert(build_stormbolt_loop_call(reinterpret_cast<std::uintptr_t>(page+4),reinterpret_cast<std::uintptr_t>(page+0x100),expected));
        std::memcpy(page+4,expected.data(),5);DWORD protection{};assert(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&protection));
        assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
        auto write_call=[&](const std::array<unsigned char,5>& bytes) {
            DWORD old{};assert(VirtualProtect(page,4096,PAGE_EXECUTE_READWRITE,&old));std::memcpy(page+4,bytes.data(),5);
            assert(VirtualProtect(page,4096,old,&old));assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
        };
        StormboltEvadeReloadEffectBindings api{reinterpret_cast<void(*)(void*)>(page+0x100),evade_request,evade_ready,&g_actor,0x1D00};
        auto requested=expected;if(evade_mode=="evade-prepare-failure") requested[1]^=1;
        binding.evade_refill_call=reinterpret_cast<std::uintptr_t>(page+4);
        binding.evade_refill_expected=requested;binding.evade_effect=api;
        Object<0x400> evade_component;Object<0x50> evade_provider,evade_dti;
        std::array<void*,34> evade_provider_table{};std::array<void*,1> evade_component_table{},evade_dti_table{};
        void* evade_singleton=&evade_provider;std::uint32_t resource_header=0xCAFE;
        auto typed_config=stormbolt_evade_effect_readiness_bindings(binding);
        if(typed_evade) {
            auto create=+[](void*,void*,void*,std::uint32_t,std::uint32_t,void*,void*,void*)->void*{return nullptr;};
            put(evade_provider,0,evade_provider_table.data());put(evade_dti,0,evade_dti_table.data());
            evade_provider_table[0x20/8]=reinterpret_cast<void*>(&evade_original);
            evade_provider_table[0xF8/8]=reinterpret_cast<void*>(&evade_request);
            evade_provider_table[0x108/8]=reinterpret_cast<void*>(create);
            binding.evade_component_vtable=reinterpret_cast<std::uintptr_t>(evade_component_table.data());
            binding.special_to_normal.provider_identity={&evade_singleton,reinterpret_cast<std::uintptr_t>(evade_provider_table.data()),
                reinterpret_cast<std::uintptr_t>(&evade_dti),reinterpret_cast<std::uintptr_t>(evade_dti_table.data()),
                reinterpret_cast<std::uintptr_t>(&evade_original),reinterpret_cast<std::uintptr_t>(&evade_request),create};
            typed_config=stormbolt_evade_effect_readiness_bindings(binding);typed_config.synchronous_borrow_verified=true;
            typed_config.resource_ready=+[](void*,void* resource,std::uint32_t group,std::uint32_t record,void*) {
                std::uint32_t header=0;return group==2000 && record==0 &&
                    read_stormbolt_field(reinterpret_cast<std::uintptr_t>(resource),0,header) && header==0xCAFE;
            };
            if(evade_mode=="evade-borrow-failure") typed_config.synchronous_borrow_verified=false;
            if(evade_mode=="evade-binding-failure") typed_config.component_vtable^=8;
            if(evade_mode=="evade-resource-failure") resource_header=0;
        }
        const auto attached=typed_evade ? g_session.attach_native_evade_effect(binding,typed_config) :
            g_session.attach_native_evade_effect(binding,evade_ready,&g_actor);
        if(evade_mode=="evade-prepare-failure" || evade_mode=="evade-borrow-failure" || evade_mode=="evade-binding-failure")
            assert(!attached.ok() && attached.issue==StormboltSessionIssue::evade_prepare);
        else {
            assert(attached.ok() && g_session.evade_effect());
            assert(g_transaction.last.owned==6 && std::memcmp(page+4,expected.data(),5)==0);
            if(evade_mode=="evade-cancel") assert(restore_evade().ok());
            else {
                if(evade_mode=="evade-commit-failure") g_transaction.fail_commit=true;
                if(evade_mode=="evade-activation-failure") {auto bad=expected;bad[0]=0xCC;write_call(bad);}
                StormboltSessionResult enabled{};
                if(coordinated) {
                    assert(!g_session.activate().ok());
                    assert(g_transaction.commit_pending_without_rollback()==MH_OK);
                    enabled=g_session.activate_after_shared_commit_while_quiescent();
                } else enabled=g_session.activate();
                assert(g_transaction.commits==1);
                if(evade_mode=="evade-commit-failure" || evade_mode=="evade-activation-failure") {
                    assert(!enabled.ok());
                    if(evade_mode=="evade-commit-failure") assert(enabled.issue==StormboltSessionIssue::activation);
                    else {
                        // The queued CALL checks its original bytes during the
                        // freeze/commit, so it must retain a foreign mutation.
                        assert(enabled.issue==StormboltSessionIssue::evade_restore);
                        assert(page[4]==0xCC && g_transaction.has_library_state());
                        write_call(expected);assert(restore_evade().ok());
                    }
                } else {
                    assert(enabled.ok());assert(native_allocate(&g_context)==&g_one);
                    put(g_one,0x1D00,std::uint32_t{24});put(g_actor,0xC,std::uint8_t{2});put(g_actor,0x8808,&g_actor);
                    if(typed_evade) {
                        assert(g_session.evade_readiness());
                        put(g_actor,0,binding.normal_to_special.actor_vtable);put(g_actor,0x76B0,&g_one);
                        put(g_actor,0x8808,&evade_component);put(evade_component,0,binding.evade_component_vtable);
                        put(evade_component,0x18,&g_actor);put(evade_component,0x30,&resource_header);
                        evade_expected_provider=&evade_component;
                    }
                    auto caller=reinterpret_cast<void(*)(void*)>(page);caller(&g_one);
                    assert(evade_originals==1 && evade_effects==(evade_mode=="evade-resource-failure" ? 0u : 1u));
                    if(evade_mode=="evade-restore") {
                        std::array<unsigned char,5> installed{};std::memcpy(installed.data(),page+4,5);
                        auto bad=installed;bad[0]=0xCC;write_call(bad);const auto stops=g_transaction.stops;
                        auto result=restore_evade();assert(!result.ok() && result.issue==StormboltSessionIssue::evade_restore);
                        assert(g_transaction.stops==stops && g_transaction.has_library_state() && g_session.evade_effect()->active());write_call(installed);
                        if(typed_evade) assert(g_session.evade_readiness());
                    }
                    if(evade_mode=="evade-stop-failure") g_transaction.fail_stop=true;
                    auto restored=restore_evade();
                    if(evade_mode=="evade-stop-failure") {
                        assert(!restored.ok() && restored.issue==StormboltSessionIssue::hook_restore && g_session.evade_effect());
                        assert(!g_session.evade_effect()->active());caller(&g_one);assert(evade_originals==2 && evade_effects==1);
                        if(typed_evade) assert(g_session.evade_readiness());
                        restored=restore_evade();
                    }
                    assert(restored.ok());
                }
            }
        }
        assert(g_session.state()==StormboltSessionState::stopped && !g_session.evade_effect() && !g_session.evade_readiness());
        assert(std::memcmp(page+4,expected.data(),5)==0);verify_restored();
        assert(VirtualFree(page,0,MEM_RELEASE));assert(VirtualFree(g_weapon_table,0,MEM_RELEASE));assert(VirtualFree(g_action_table,0,MEM_RELEASE));
        std::printf("PASS shared evade CALL transaction / %s\n",mode.c_str());return 0;
    }
    const bool producer=mode.rfind("full-update-power-cell-producer",0)==0;
    const bool producer_failure=mode=="full-update-power-cell-producer-failure";
    const bool growth=mode.rfind("full-update-power-cell-producer-growth",0)==0;
    const unsigned growth_conflict=growth && mode.back()>='1' && mode.back()<='3' ? unsigned(mode.back()-'0') : 0;
    const auto previous_growth=stormbolt_growth_dispatch_target;
    const std::array<unsigned char,8> growth_a{{0x85,0xF6,0x0F,0x88,0x96,0,0,0}};
    const std::array<unsigned char,9> growth_b{{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76}};
    const std::array<unsigned char,12> growth_c{{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0}};
    std::array<std::uintptr_t,3> growth_sites{};
    unsigned char* producer_page=nullptr;
    const std::array<unsigned char,6> pack_original{{0x89,0x7D,0xA7,0x4C,0x2B,0xC0}};
    const std::array<unsigned char,8> complete_original{{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0}};
    const auto previous_pack=stormbolt_sync_pack_dispatch_target;
    const auto previous_complete=stormbolt_sync_complete_dispatch_target;
    std::array<unsigned char,7> status_original{},loop_original{},timer_original{};
    if (power_cell) {
        assert(prepared.ok());
        put(pc_owner,0x60,std::uint8_t{1});put(pc_owner,0x68,1.0f);
        put(pc_owner,0x126D8,static_cast<void*>(&pc_skills));put(pc_owner,0x7D20,static_cast<void*>(&pc_status));
        put(pc_skills,0,std::uintptr_t{1});put(pc_skills,0x141C,std::int32_t{3});
        put(pc_resource,0,std::uintptr_t{3});put(pc_resource,0xA8C,std::uint32_t{4});
        binding.power_cell={reinterpret_cast<std::uintptr_t>(&pc_construct),reinterpret_cast<std::uintptr_t>(&pc_reset),
            reinterpret_cast<std::uintptr_t>(&pc_destroy),2,0,0};
        binding.loop_construct=reinterpret_cast<std::uintptr_t>(&pc_loop_construct);
        binding.loop_destroy=reinterpret_cast<std::uintptr_t>(&pc_loop_destroy);binding.loop_vtable=4;
        binding.loop_inputs={1,2,reinterpret_cast<std::uintptr_t>(&pc_slot),3};
        binding.skill_value_query=+[](void*,std::uint32_t,std::uint32_t,std::int32_t) {return 1;};
        binding.power_cell_material_at=+[](void*,std::uint32_t)->void* {return nullptr;};
        binding.power_cell_status_update_site=reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_patch_site);
        binding.power_cell_status_manager_slot=reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_manager);
        binding.loop_update_site=mode=="full-update-power-cell-activation-failure"
            ?binding.power_cell_status_update_site:reinterpret_cast<std::uintptr_t>(&stormbolt_loop_test_patch_site);
        std::memcpy(status_original.data(),&stormbolt_status_test_patch_site,7);
        std::memcpy(loop_original.data(),&stormbolt_loop_test_patch_site,7);
        binding.power_cell_timer_update_site=reinterpret_cast<std::uintptr_t>(&stormbolt_timer_test_patch_site);
        binding.power_cell_timer_native_query=&stormbolt_timer_test_query;
        binding.skill_prompt=+[](void*,std::uint32_t,bool,std::int32_t) {};
        std::memcpy(timer_original.data(),&stormbolt_timer_test_patch_site,7);
        damage_session_fixture::configure(binding);
        StormboltSyncPackBindings pack{};
        if (producer) {
            producer_page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
            assert(producer_page);
            std::memcpy(producer_page,pack_original.data(),6);
            std::memcpy(producer_page+64,complete_original.data(),8);
            if (producer_failure) producer_page[0]=0xCC;
            binding.power_cell_sync_construct=reinterpret_cast<std::uintptr_t>(&pc_sync_construct);
            binding.power_cell_sync_destroy=reinterpret_cast<std::uintptr_t>(&pc_sync_destroy);
            binding.power_cell_sync_vtable=77;
            binding.power_cell_sync_pack_site=reinterpret_cast<std::uintptr_t>(producer_page);
            binding.power_cell_sync_complete_site=reinterpret_cast<std::uintptr_t>(producer_page+64);
            pack={77,binding.loop_inputs,binding.skill_value_query,&encode_stormbolt_power_cell_sync};
        }
        StormboltPowerCellCompleteGrowthBindings growth_bindings{};
        if(growth) {
            binding.class48_skill_multiplier=reinterpret_cast<std::uintptr_t>(&pc_skill_multiplier);
            binding.skill_value_query=+[](void*,std::uint32_t id,std::uint32_t,std::int32_t) {return id==70?120:1;};
            static void* manager=nullptr;
            binding.gatling_entry.player_manager_slot=&manager;
            binding.gatling_entry.local_player=+[](void*)->void* {return nullptr;};
            binding.equipment_effect=+[](void*,std::uint32_t,std::uint32_t,std::uint32_t,bool)->void* {return nullptr;};
            growth_bindings=stormbolt_complete_growth_bindings(binding);
            growth_sites={{reinterpret_cast<std::uintptr_t>(producer_page+128),reinterpret_cast<std::uintptr_t>(producer_page+160),reinterpret_cast<std::uintptr_t>(producer_page+192)}};
            std::memcpy(producer_page+128,growth_a.data(),8);
            std::memcpy(producer_page+160,growth_b.data(),9);
            std::memcpy(producer_page+192,growth_c.data(),12);
            if(growth_conflict) *reinterpret_cast<unsigned char*>(growth_sites[growth_conflict-1])=0xCC;
        }
        const auto attached_power=g_session.attach_power_cell(binding,status_original,timer_original,{},pack,growth_sites,growth_bindings);
        if(producer_failure || growth_conflict) {
            assert(!attached_power.ok() && attached_power.issue==StormboltSessionIssue::power_cell_prepare);
            prepared=attached_power;
        } else assert(attached_power.ok());
    }
    if(mode.rfind("source-",0)==0) {
        assert(prepared.ok());
        const bool manager=mode.rfind("source-manager",0)==0;
        const bool native_ready=mode.find("-native-")!=std::string::npos;
        const bool condition=mode.rfind("source-condition",0)==0 || manager;
        source_condition_table[6]=reinterpret_cast<void*>(&source_condition_original);
        source_dti_table[1]=reinterpret_cast<void*>(&source_original_create);
        binding.source_transition_type=source_type.data();
        binding.source_transition_heap=&source_heap;
        binding.source_transition={reinterpret_cast<std::uintptr_t>(source_dti_table.data()),
            &source_original_create,&source_place,&source_destroy};
        if(mode=="source-prepare-failure")
            binding.source_transition.original_destroy=reinterpret_cast<decltype(binding.source_transition.original_destroy)>(&source_place);
        const auto attached=g_session.attach_source_transition(binding,&source_allocate);
        if(mode=="source-prepare-failure") {
            assert(!attached.ok() && attached.issue==StormboltSessionIssue::source_transition_prepare);
            assert(g_session.state()==StormboltSessionState::stopped && !g_session.source_transition());
        } else {
            assert(attached.ok() && g_transaction.last.owned==7);
            if(condition) {
                binding.source_transition_condition={reinterpret_cast<std::uintptr_t>(source_condition_table.data()),
                    reinterpret_cast<std::uintptr_t>(g_actor_table.data()),reinterpret_cast<std::uintptr_t>(g_weapon_table),
                    &source_condition_original,nullptr};
                const StormboltSourceTransitionPredicateApi predicate{
                    +[](void*)->std::int32_t{return 0;},+[](void*){return false;},+[](){return false;},+[](){return true;},
                    +[](void*)->std::uint32_t{return 1;},+[](void*){return false;},+[](void*,float&){return false;},
                    +[](void*)->std::int32_t{++source_command_calls;return 0;},+[](void*){return false;}};
                if(!native_ready) assert(g_session.attach_source_condition(binding,predicate,+[](void*,void*){return true;}).ok());
            }
            if(manager) {
                auto manager_mode=mode;
                if(native_ready) manager_mode.erase(manager_mode.find("-native"),7);
                const auto manager_vtable=reinterpret_cast<std::uintptr_t>(manager_base_table.data());
                put(manager_object,0,manager_vtable);put(manager_object,0xD90,&pc_owner);
                put(pc_owner,0,binding.normal_to_special.actor_vtable);put(pc_owner,0xC,std::uint8_t{2});
                put(pc_owner,0x76B0,&g_one);put(pc_owner,0x12608,&manager_object);put(pc_owner,0x14F8,&manager_object);
                manager_original_links.reserve(std::size(source_graph_edits));
                for(const auto& edit:source_graph_edits) {
                    manager_original_links.emplace_back(edit.expected,edit.expected+edit.count);
                    manager_rows[edit.state].lists[edit.secondary]={manager_original_links.back().data(),static_cast<std::uint32_t>(edit.count),0};
                }
                const auto rows_before=manager_rows;
                manager_registry[776]=source_storage.data();
                put(manager_object,0xAB0,manager_rows.data());put(manager_object,0xAB8,std::uint32_t{120});
                put(manager_object,0xAD0,manager_registry.data());put(manager_object,0xAD8,std::uint32_t{798});
                binding.source_manager_vtable=manager_vtable;
                binding.source_manager={&session_manager_initialize,&session_manager_rebuild,&session_manager_tick,&session_manager_destroy};
                binding.shoot_condition={source_type.data(),&manager_condition_create,&manager_condition_initialize,
                    &manager_condition_destroy,manager_base_table.data(),binding.normal_to_special.actor_vtable,binding.shell.weapon_vtable};
                binding.local_input.query=+[](void*,std::uint32_t){return false;};
                binding.input_trigger.primary=+[](void*,std::uint32_t){return false;};
                if(native_ready) {
                    put(pc_owner,0x14F8,&manager_input_root);put(manager_input_root,0x40,&pc_owner);
                    put(manager_object,0x40,&pc_owner);put(pc_owner,0xE480,&manager_physical);
                    put(pc_owner,0x7658,&manager_component);put(pc_owner,0x7D20,&manager_status);
                    put(pc_owner,0x7810,&g_parameters);put(g_parameters,0,binding.coefficients.parameter_vtable);
                    put(manager_players,0xAE40,std::uint64_t{123});put(manager_players,0x48,std::uint64_t{123});put(manager_players,0x58,&pc_owner);
                    binding.gatling_entry.player_manager_slot=&manager_players_slot;
                    binding.update_gate={&manager_authority_slot,+[](void*,std::uint32_t)->std::uint8_t{return 1;}};
                    binding.source_command_system_slot=&manager_system_slot;
                    binding.source_mine_enabled=+[](void*){return false;};
                    binding.source_command=+[](void*)->std::int32_t{return 0;};
                    binding.source_platform_blocked=+[]{return false;};
                }
                if(manager_mode=="source-manager-prepare-failure")
                    binding.source_manager.destroy=reinterpret_cast<decltype(binding.source_manager.destroy)>(&session_manager_rebuild);
                auto readiness=stormbolt_condition_readiness_bindings(binding);
                readiness.synchronous_borrow_verified=manager_mode!="source-manager-borrow-failure";
                if(manager_mode=="source-manager-binding-failure") readiness.weapon_vtable^=8;
                const auto result=native_ready ? g_session.attach_native_source_conditions(binding,readiness) :
                    g_session.attach_source_manager(binding,+[](void*,void*,void*){return false;});
                if(manager_mode=="source-manager-borrow-failure" || manager_mode=="source-manager-binding-failure") {
                    assert(!result.ok() && result.issue==StormboltSessionIssue::source_condition_prepare);
                    assert(g_session.state()==StormboltSessionState::stopped && !g_session.source_readiness());
                } else if(manager_mode=="source-manager-prepare-failure") {
                    assert(!result.ok() && result.issue==StormboltSessionIssue::source_manager_prepare);
                    assert(g_session.state()==StormboltSessionState::stopped && !g_session.source_manager());
                } else if(manager_mode=="source-manager-cancel") {
                    assert(result.ok() && g_transaction.last.owned==11);
                    assert(g_session.restore().ok());
                } else {
                    assert(result.ok() && g_transaction.last.owned==11);
                    if(manager_mode=="source-manager-commit-failure") {
                        g_transaction.fail_commit=true;g_transaction.apply_before_failed_commit=true;
                        g_transaction.before_failed_commit=+[] {
                            assert(native_allocate(&g_context)==&g_one);put(g_one,0x9B0,&pc_owner);
                            session_manager_initialize(&manager_object);
                            assert(g_session.source_manager()->active_for(&manager_object));
                        };
                    }
                    const auto activated=g_session.activate();
                    if(manager_mode=="source-manager-commit-failure") {
                        assert(!activated.ok() && activated.issue==StormboltSessionIssue::activation);
                        assert(g_session.state()==StormboltSessionState::stopped);
                    } else {
                        assert(activated.ok());
                        assert(native_allocate(&g_context)==&g_one);put(g_one,0x9B0,&pc_owner);
                        session_manager_initialize(&manager_object);
                        assert(g_session.source_manager()->active_for(&manager_object));
                        if(native_ready) {
                            assert(g_session.source_readiness() && g_session.source_readiness()->shoot(manager_condition_storage.data(),&g_one));
                            auto* setbomb=reinterpret_cast<void*(*)(void*)>(source_dti_table[1])(source_type.data());
                            manager_condition_initialize(setbomb,&pc_owner);
                            put(g_one,0x2848,std::int32_t{1});
                            put(g_one,0x2388,std::uint32_t{0});
                            assert(g_session.source_readiness()->setbomb(setbomb,&g_one));
                            const auto original_before=source_condition_original_calls;
                            const auto commands_before=source_command_calls;
                            assert(reinterpret_cast<std::int32_t(*)(void*)>(source_condition_table[6])(setbomb)==-7);
                            assert(source_condition_original_calls==original_before+1 && source_command_calls==commands_before);
                            assert(source_destroy(setbomb,1)==setbomb);
                        }
                        if(manager_mode=="source-manager-restore") {
                            const auto& edit=source_graph_edits[0];auto& slot=manager_rows[edit.state].lists[edit.secondary];
                            auto* published=slot.links;slot.links=nullptr;
                            const auto stopped_before=g_transaction.stops;
                            const auto failed=g_session.restore();
                            assert(!failed.ok() && failed.issue==StormboltSessionIssue::source_manager_restore);
                            assert(g_transaction.stops==stopped_before && g_transaction.has_library_state());
                            assert(g_session.source_transition()->factory_active() && manager_condition_destroys==0);
                            if(native_ready) assert(g_session.source_readiness());
                            slot.links=published;
                        }
                        if(manager_mode=="source-manager-stop-failure") g_transaction.fail_stop=true;
                        const auto restored=g_session.restore();
                        if(manager_mode=="source-manager-stop-failure") {
                            assert(!restored.ok() && restored.issue==StormboltSessionIssue::hook_restore);
                            assert(g_session.source_manager() && !g_session.source_manager()->active_for(&manager_object));
                            if(native_ready) assert(g_session.source_readiness());
                            session_manager_tick(&manager_object);
                            assert(!g_session.source_manager()->active_for(&manager_object));
                            assert(g_session.restore().ok());
                        } else assert(restored.ok());
                    }
                    assert(manager_condition_destroys==1);
                }
                assert(stormbolt_source_field<void**>(&manager_object,0xAD0)==manager_registry.data());
                assert(stormbolt_source_field<std::uint32_t>(&manager_object,0xAD8)==798);
                assert(!std::memcmp(manager_rows.data(),rows_before.data(),sizeof(manager_rows)));
                verify_restored();
                assert(!g_transaction.has_library_state() && !g_session.source_manager() && !g_session.source_readiness());
                std::printf("PASS: shared source-manager transaction / %s\n",mode.c_str());return 0;
            }
            if(mode=="source-cancel" || mode=="source-condition-cancel") assert(g_session.restore().ok());
            else {
                if(mode=="source-commit-failure" || mode=="source-condition-commit-failure") g_transaction.fail_commit=true;
                if(mode=="source-activation-failure") source_dti_table[1]=reinterpret_cast<void*>(&foreign_action);
                if(mode=="source-condition-activation-failure") source_condition_table[6]=reinterpret_cast<void*>(&source_condition_foreign);
                const auto activated=g_session.activate();
                if(mode=="source-commit-failure" || mode=="source-activation-failure" || mode=="source-condition-commit-failure" || mode=="source-condition-activation-failure") {
                    assert(!activated.ok() && g_session.state()==StormboltSessionState::stopped);
                    assert(activated.issue==((mode=="source-commit-failure" || mode=="source-condition-commit-failure") ? StormboltSessionIssue::activation : mode=="source-condition-activation-failure" ? StormboltSessionIssue::source_condition_activation : StormboltSessionIssue::source_transition_activation));
                } else {
                    assert(activated.ok());
                    auto invoke=[&](){return reinterpret_cast<void* (*)(void*)>(source_dti_table[1])(source_type.data());};
                    auto* object=invoke();assert(object==source_storage.data());
                    StormboltInstanceToken token{};
                    assert(g_session.source_transition()->instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object),token));
                    if(condition) {
                        assert(native_allocate(&g_context)==&g_one);
                        put(g_actor,0x76B0,&g_one);
                        auto* actor_pointer=&g_actor;
                        std::memcpy(source_storage.data()+0x10,&actor_pointer,sizeof(actor_pointer));
                        auto call_condition=[&]() {return reinterpret_cast<std::int32_t (*)(void*)>(source_condition_table[6])(object);};
                        assert(call_condition()==2 && source_command_calls==1 && source_condition_original_calls==0);
                    }
                    assert(source_destroy(object,1)==object);
                    StormboltSourceTransitionState snapshot{};
                    assert(!g_session.source_transition()->instances().snapshot(token,snapshot));
                    if(mode=="source-restore") {
                        const auto installed=source_dti_table[1];
                        source_dti_table[1]=reinterpret_cast<void*>(&foreign_action);
                        const auto result=g_session.restore();
                        assert(!result.ok() && result.issue==StormboltSessionIssue::source_transition_restore);
                        assert(g_transaction.has_library_state() && g_session.source_transition());
                        source_dti_table[1]=installed;
                    }
                    if(mode=="source-condition-restore") {
                        const auto installed=source_condition_table[6];
                        source_condition_table[6]=reinterpret_cast<void*>(&source_condition_foreign);
                        const auto failed=g_session.restore();
                        assert(!failed.ok() && failed.issue==StormboltSessionIssue::source_condition_restore);
                        assert(g_transaction.has_library_state() && g_session.source_transition()->factory_active());
                        source_condition_table[6]=installed;
                    }
                    assert(g_session.restore().ok());
                    if(condition) assert(source_condition_table[6]==reinterpret_cast<void*>(&source_condition_original));
                }
            }
            assert(!g_session.source_transition());
        }
        verify_restored();
        assert(VirtualFree(g_weapon_table,0,MEM_RELEASE));
        assert(VirtualFree(g_action_table,0,MEM_RELEASE));
        std::printf("PASS: shared transaction session / %s\n",mode.c_str());
        return 0;
    }
    if (full) binding.special_to_normal = {}; // All callbacks must survive input destruction.
    if(coordinated) {
        // The quest fixture shares this transaction: shooting must never commit
        // or stop it independently, including failed prepare and commit paths.
        assert(g_transaction.has_library_state() && g_transaction.stops==0);
        assert(!g_session.activate().ok() && !g_session.restore().ok());
        assert(!g_session.activate_after_shared_commit_while_quiescent().ok());
        assert(!g_session.release_after_shared_stop_while_quiescent().ok());
        const bool commit_failure=mode=="commit-failure";
        if(prepared.ok()) {
            g_transaction.fail_commit=commit_failure;
            g_transaction.apply_before_failed_commit=commit_failure;
            const auto committed=g_transaction.commit_pending_without_rollback();
            assert(g_transaction.commits==1);
            if(commit_failure) assert(committed==MH_ERROR_MEMORY_PROTECT);
            else {
                assert(committed==MH_OK);
                const auto activated=g_session.activate_after_shared_commit_while_quiescent();
                if(producer_failure || growth_conflict) {
                    assert(!activated.ok() && activated.issue==StormboltSessionIssue::power_cell_activation);
                    assert(g_session.state()==StormboltSessionState::recovery_required);
                    assert(g_transaction.stops==0 && g_session.power_cell());
                } else {
                    assert(activated.ok());
                    assert(native_allocate(&g_context)==&g_one);
                    transition(g_one);
                    if(full) full_transition();
                    if(mode=="full-middle-chain") {
                        for(unsigned i=0;i<9;++i) chain_shot(i);
                        start_transition();
                        std::puts("PASS:shared host executes installed mode/fire chain before restoration");
                    }
                    assert(native_destroy(&g_one,5)==&g_one);
                }
                assert(g_transaction.commits==1);
            }
            assert(quest_native(1)==13);
        } else {
            assert(mode=="add-failure" || mode=="slot-failure" || producer_failure || growth_conflict);
            assert(g_session.state()==StormboltSessionState::recovery_required);
            assert(g_transaction.commits==0 && quest_native(1)==11);
        }
        assert(g_session.restore_before_shared_stop_while_quiescent().ok());
        assert(g_transaction.stops==0 && g_transaction.has_library_state());
        assert(quest_native(1)==(prepared.ok()?13:11));
        assert(!g_session.release_after_shared_stop_while_quiescent().ok());
        g_transaction.fail_stop=true;
        assert(g_transaction.stop()==MH_ERROR_MEMORY_PROTECT);
        assert(!g_session.release_after_shared_stop_while_quiescent().ok());
        assert(g_session.restore_before_shared_stop_while_quiescent().ok());
        assert(g_transaction.stop()==MH_OK);
        assert(g_session.release_after_shared_stop_while_quiescent().ok());
        assert(g_session.release_after_shared_stop_while_quiescent().ok());
        verify_restored();
    } else if (producer_failure || growth_conflict || mode == "add-failure" || mode == "slot-failure" || mode == "action-slot-failure" || mode == "full-slot-failure" || mode == "full-start-slot-failure" || mode == "full-middle-slot-failure" || mode == "full-last-slot-failure" || mode == "full-update-slot-failure") {
        assert(!prepared.ok() && prepared.state == StormboltSessionState::stopped);
        if(mode=="full-update-slot-failure") assert(prepared.issue==StormboltSessionIssue::update_pointer);
        if(mode=="full-last-slot-failure") assert(prepared.issue==StormboltSessionIssue::last_pointer);
        if(mode=="full-middle-slot-failure") assert(prepared.issue==StormboltSessionIssue::middle_pointer);
        if (mode == "full-start-slot-failure") assert(prepared.issue == StormboltSessionIssue::start_pointer);
        if (mode == "full-slot-failure") assert(prepared.issue == StormboltSessionIssue::special_action_pointer);
        if (mode == "action-slot-failure") assert(prepared.issue == StormboltSessionIssue::action_pointer);
        verify_restored();
    } else {
        assert(prepared.ok() && g_session.state() == StormboltSessionState::prepared);
        if(update) assert(g_weapon_table[0x228/8]!=reinterpret_cast<std::uintptr_t>(&start_original_entry));
        assert(quest_native(1) == 11 && g_transaction.last.owned == (power_cell?(producer?(growth?25:21):17):5));
        assert(g_weapon_table[123] != reinterpret_cast<std::uintptr_t>(&coefficient_original));
        assert(g_action_table[7] != reinterpret_cast<std::uintptr_t>(&action_original));
        if (mode == "cancel" || mode == "full-cancel" || mode == "full-middle-cancel" || mode == "full-last-cancel" || mode == "full-update-cancel" || mode=="full-update-power-cell-cancel") {
            assert(g_session.restore().ok()); verify_restored();
        } else {
            g_transaction.fail_commit = mode == "full-commit-failure" || mode == "full-commit-restore" || mode == "commit-failure" || mode == "commit-restore-conflict" || mode == "commit-action-restore-conflict" || mode=="full-update-power-cell-commit-failure";
            const auto prepared_slot = g_weapon_table[123];
            const auto prepared_action_slot = g_action_table[7];
            const auto stop_count_before_commit = g_transaction.stops;
            if (mode == "commit-restore-conflict") {
                g_transaction.apply_before_failed_commit = true;
                g_transaction.before_failed_commit = [] {
                    exchange_fixture_slot(reinterpret_cast<std::uintptr_t>(&foreign_callback));
                };
            }
            if (mode == "commit-action-restore-conflict") {
                g_transaction.apply_before_failed_commit = true;
                g_transaction.before_failed_commit = [] {
                    exchange_action_slot(reinterpret_cast<std::uintptr_t>(&foreign_action));
                };
            }
            const auto special_exit = g_special_table[8];
            if (mode == "full-commit-restore") {
                g_transaction.apply_before_failed_commit = true;
                g_transaction.before_failed_commit = [] { g_special_table[8] = reinterpret_cast<std::uintptr_t>(&foreign_action); };
            }
            const auto activated = g_session.activate();
            if (mode == "full-commit-restore") {
                assert(activated.issue == StormboltSessionIssue::special_action_restore);
                assert(g_session.state() == StormboltSessionState::recovery_required);
                assert(g_transaction.stops == stop_count_before_commit && quest_native(1) == 13);
                assert(g_action_table[7] == prepared_action_slot && g_weapon_table[123] == prepared_slot);
                assert(native_allocate(&g_context) == &g_one);
                g_special_table[8] = special_exit;
                full_transition();
                assert(g_session.restore().ok()); verify_restored();
            } else
            if (mode == "commit-action-restore-conflict") {
                assert(!activated.ok() && activated.issue == StormboltSessionIssue::action_pointer_restore);
                assert(g_session.state() == StormboltSessionState::recovery_required);
                assert(g_transaction.has_library_state() && g_transaction.stops == stop_count_before_commit);
                assert(quest_native(1) == 13);
                assert(g_weapon_table[123] == prepared_slot); // Must not restore earlier-installed coefficient yet.
                assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&foreign_action));
                exchange_action_slot(prepared_action_slot);
                assert(g_session.restore().ok()); verify_restored();
            } else if (mode == "commit-restore-conflict") {
                assert(!activated.ok() && activated.issue == StormboltSessionIssue::pointer_restore);
                assert(g_session.state() == StormboltSessionState::recovery_required);
                assert(g_transaction.has_library_state() && g_transaction.stops == stop_count_before_commit);
                assert(quest_native(1) == 13); // Applied hooks must remain until the pointer is restored.
                assert(g_weapon_table[123] == reinterpret_cast<std::uintptr_t>(&foreign_callback));
                assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&action_original));
                exchange_fixture_slot(prepared_slot);
                assert(g_session.restore().ok()); verify_restored();
            } else if (mode=="full-update-power-cell-activation-failure" || producer_failure || growth_conflict) {
                assert(!activated.ok() && activated.issue==StormboltSessionIssue::power_cell_activation);
                assert(g_session.state()==StormboltSessionState::stopped);verify_restored();
            } else if (mode == "commit-failure" || mode == "full-commit-failure" || mode=="full-update-power-cell-commit-failure") {
                assert(!activated.ok() && activated.issue == StormboltSessionIssue::activation);
                verify_restored();
            } else {
                assert(activated.ok() && g_session.state() == StormboltSessionState::enabled);
                if (power_cell) {
                    if(growth) for(auto site:growth_sites) assert(*reinterpret_cast<unsigned char*>(site)==0xE8);
                    if (producer) assert(producer_page[0]==0xE8 && producer_page[64]==0xE8);
                    auto* pc=g_session.power_cell();assert(pc);
                    // Published CALLs must keep their dispatch owners. The
                    // subsequent native fixture calls verify rejection is inert.
                    assert(!pc->withdraw_updates_after_quiescence());
                    assert(pc_construct(&pc_status,&pc_owner)==&pc_status);
                    if(growth) {
                        StormboltNativeSkillMultiplier volatile multiplier=&pc_skill_multiplier;
                        skill_multiplier_calls=0;
                        assert(multiplier(&pc_status,10.0f,48)==24.0f && skill_multiplier_calls==1);
                        assert(multiplier(&pc_status,10.0f,17)==20.0f && skill_multiplier_calls==2);
                    }
                    assert(pc_loop_construct(&pc_loop)==&pc_loop);
                    StormboltPowerCellToken token{};StormboltPowerCellLoopToken loop_token{};
                    assert(pc->statuses().token_for_live_address(reinterpret_cast<std::uintptr_t>(&pc_status),token));
                    assert(pc->loops().token_for_live_address(reinterpret_cast<std::uintptr_t>(&pc_loop),loop_token));
                    assert(pc->statuses().transact(token,[](auto& s) noexcept {s.count=1;return true;}));
                    std::array<std::uint64_t,24> result{};stormbolt_status_test_call(&pc_status,result.data());
                    StormboltPowerCellState state{};assert(pc->statuses().snapshot(token,state) && state.event_eligible);
                    assert(pc->loops().transact(loop_token,[](auto& s) noexcept {s.growth=true;return true;}));
                    stormbolt_loop_test_call(&pc_loop,result.data());
                    StormboltPowerCellLoopState loop{};assert(pc->loops().snapshot(loop_token,loop) && loop.visual_phase==1 && !loop.growth);
                    pc_destroy(&pc_status);pc_loop_destroy(&pc_loop,5);
                }
                assert(quest_native(1) == 13);
                assert(native_allocate(&g_context) == &g_one && g_last_type == &g_context);
                assert(native_place(&g_context, &g_two) == &g_two && g_last_type == &g_context);
                const auto one = token_for(&g_one), two = token_for(&g_two);
                transition(g_one); transition(g_two);
                if (full) full_transition();
                if (start && mode!="full-middle-chain") start_transition();
                StormboltStageTiming timing{}; timing.post_shot_decay = 0.5f;
                for (unsigned i = 0; i < 9; ++i) {
                    if(mode=="full-middle-chain") {
                        chain_shot(i);
                    } else {
                        // Direct stage fixture for lifecycle/failure modes.
                        assert(g_session.instances().transact(one, [&](auto& state) noexcept {
                            return advance_stormbolt_stage(state, timing);
                        }));
                        fire(g_one, i);
                    }
                    fire(g_two, 0);
                }
                if(mode=="full-middle-chain") {
                    start_transition();
                    std::puts("PASS:installed mode-entry ->primary/middle preparation ->9 consumed events ->shell/coefficients ->mode-exit; fixture native APIs");
                }
                assert(native_destroy(&g_one, 5) == &g_one && g_flags == 5);
                put(g_actor, 0x76B0, &g_one);
                const auto queries_after_destroy = g_query_calls;
                assert(!reinterpret_cast<StormboltActionUpdate>(g_action_table[7])(&g_action));
                assert(g_query_calls == queries_after_destroy);
                assert(native_place(&g_context, &g_one) == &g_one);
                transition(g_one);
                const auto replacement = token_for(&g_one);
                assert(replacement.generation != one.generation);
                StormboltStageState state{};
                assert(!g_session.instances().snapshot(one, state));
                assert(g_session.instances().snapshot(two, state) && state.counter == 0);
                fire(g_one, 0);
                if (mode == "restore-conflict") {
                    const auto ours = g_weapon_table[123]; const auto stop_count = g_transaction.stops;
                    exchange_fixture_slot(reinterpret_cast<std::uintptr_t>(&foreign_callback));
                    const auto failed = g_session.restore();
                    assert(!failed.ok() && failed.issue == StormboltSessionIssue::pointer_restore);
                    assert(g_transaction.has_library_state() && g_transaction.stops == stop_count);
                    assert(g_weapon_table[123] == reinterpret_cast<std::uintptr_t>(&foreign_callback));
                    assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&action_original));
                    exchange_fixture_slot(ours); // Synthetic external writer resolves its ownership conflict.
                }
                if (mode == "action-restore-conflict") {
                    const auto ours = g_action_table[7], coefficient = g_weapon_table[123];
                    const auto stop_count = g_transaction.stops;
                    exchange_action_slot(reinterpret_cast<std::uintptr_t>(&foreign_action));
                    const auto failed = g_session.restore();
                    assert(!failed.ok() && failed.issue == StormboltSessionIssue::action_pointer_restore);
                    assert(g_transaction.has_library_state() && g_transaction.stops == stop_count);
                    assert(g_weapon_table[123] == coefficient);
                    assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&foreign_action));
                    exchange_action_slot(ours);
                }
                if (mode == "full-restore") {
                    const auto ours = g_special_table[8], normal = g_action_table[7], coefficient = g_weapon_table[123];
                    const auto stops = g_transaction.stops;
                    g_special_table[8] = reinterpret_cast<std::uintptr_t>(&foreign_action);
                    const auto failed = g_session.restore();
                    assert(failed.issue == StormboltSessionIssue::special_action_restore);
                    assert(g_transaction.stops == stops && g_transaction.has_library_state());
                    assert(g_action_table[7] == normal && g_weapon_table[123] == coefficient);
                    // The complete core and stable resolver remain callable while restoration waits.
                    g_special_motion = g_special_create = g_special_save = 0;
                    g_special_table[8] = ours;
                    full_transition();
                }
                if(mode=="full-update-restore") {
                    const auto owned=g_weapon_table[0x228/8];
                    const auto last_slots=last_table, middle_slots=middle_table;
                    const auto start_slot=g_start_table[7], special_slot=g_special_table[7];
                    const auto normal_slot=g_action_table[7], coefficient_slot=g_weapon_table[123];
                    const auto stops=g_transaction.stops;
                    auto exchange_update=[](std::uintptr_t value) {
                        DWORD protection=0, ignored=0;
                        assert(VirtualProtect(g_weapon_table,4096,PAGE_READWRITE,&protection));
                        g_weapon_table[0x228/8]=value;
                        assert(VirtualProtect(g_weapon_table,4096,protection,&ignored));
                    };
                    exchange_update(reinterpret_cast<std::uintptr_t>(&foreign_action));
                    assert(g_session.restore().issue==StormboltSessionIssue::update_restore);
                    assert(g_session.state()==StormboltSessionState::recovery_required);
                    assert(g_transaction.stops==stops && g_transaction.has_library_state());
                    assert(last_table==last_slots && middle_table==middle_slots);
                    assert(g_start_table[7]==start_slot && g_special_table[7]==special_slot);
                    assert(g_action_table[7]==normal_slot && g_weapon_table[123]==coefficient_slot);
                    exchange_update(owned);
                }
                if(mode=="full-last-restore") {
                    const auto owned=last_table[8], middle_slot=middle_table[7], start_slot=g_start_table[7];
                    const auto stops=g_transaction.stops;
                    last_table[8]=reinterpret_cast<std::uintptr_t>(&foreign_action);
                    assert(g_session.restore().issue==StormboltSessionIssue::last_restore);
                    assert(g_transaction.stops==stops && middle_table[7]==middle_slot && g_start_table[7]==start_slot);
                    last_table[8]=owned;
                }
                if (mode == "full-middle-restore") {
                    const auto ours=middle_table[7], start_slot=g_start_table[7], special_slot=g_special_table[7];
                    const auto stops=g_transaction.stops;
                    middle_table[7]=reinterpret_cast<std::uintptr_t>(&foreign_action);
                    assert(g_session.restore().issue==StormboltSessionIssue::middle_restore);
                    assert(g_transaction.stops==stops && g_start_table[7]==start_slot && g_special_table[7]==special_slot);
                    middle_table[7]=ours;
                }
                if (mode == "full-start-restore") {
                    const auto ours=g_start_table[7], special=g_special_table[7], normal=g_action_table[7];
                    const auto stops=g_transaction.stops;
                    g_start_table[7]=reinterpret_cast<std::uintptr_t>(&foreign_action);
                    assert(g_session.restore().issue==StormboltSessionIssue::start_restore);
                    assert(g_transaction.stops==stops && g_special_table[7]==special && g_action_table[7]==normal);
                    g_start_table[7]=ours;
                    start_transition();
                }
                if (mode == "stop-failure") {
                    g_transaction.fail_stop = true;
                    const auto failed = g_session.restore();
                    assert(!failed.ok() && failed.issue == StormboltSessionIssue::hook_restore);
                    assert(g_transaction.has_library_state() && g_session.state() == StormboltSessionState::recovery_required);
                    assert(g_weapon_table[123] == reinterpret_cast<std::uintptr_t>(&coefficient_original));
                    assert(g_action_table[7] == reinterpret_cast<std::uintptr_t>(&action_original));
                }
                assert(g_session.restore().ok()); verify_restored();
            }
        }
    }
    if (power_cell) {
        assert(!g_session.power_cell());
        assert(!std::memcmp(status_original.data(),&stormbolt_status_test_patch_site,7));
        assert(!std::memcmp(loop_original.data(),&stormbolt_loop_test_patch_site,7));
        assert(!std::memcmp(timer_original.data(),&stormbolt_timer_test_patch_site,7));
    }
    if(growth) {
        StormboltNativeSkillMultiplier volatile multiplier=&pc_skill_multiplier;
        skill_multiplier_calls=0;assert(multiplier(&pc_status,10.0f,48)==20.0f && skill_multiplier_calls==1);
        assert(stormbolt_growth_dispatch_target==previous_growth);
        for(unsigned n=0;n<3;++n) {
            auto* site=reinterpret_cast<void*>(growth_sites[n]);
            if(growth_conflict==n+1) assert(*static_cast<unsigned char*>(site)==0xCC);
            else if(n==0) assert(!std::memcmp(site,growth_a.data(),8));
            else if(n==1) assert(!std::memcmp(site,growth_b.data(),9));
            else assert(!std::memcmp(site,growth_c.data(),12));
        }
    }
    if (producer) {
        assert(stormbolt_sync_pack_dispatch_target==previous_pack && stormbolt_sync_complete_dispatch_target==previous_complete);
        assert(!std::memcmp(producer_page+64,complete_original.data(),8));
        if (producer_failure) assert(producer_page[0]==0xCC);
        else assert(!std::memcmp(producer_page,pack_original.data(),6));
        assert(VirtualFree(producer_page,0,MEM_RELEASE));
    }
    assert(primary_table[6]==reinterpret_cast<std::uintptr_t>(&start_original_entry) &&
        primary_table[7]==reinterpret_cast<std::uintptr_t>(&start_original_update));
    assert(VirtualFree(g_weapon_table, 0, MEM_RELEASE));
    assert(VirtualFree(g_action_table, 0, MEM_RELEASE));
    std::printf("PASS: shared transaction session / %s\n", mode.c_str());
}


