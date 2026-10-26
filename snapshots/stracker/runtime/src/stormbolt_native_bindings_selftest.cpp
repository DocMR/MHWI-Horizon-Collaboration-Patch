#ifdef NDEBUG
#undef NDEBUG // Selftests must execute assertions in Release builds too.
#endif
#include "stormbolt_native_bindings.h"
#include "stormbolt_runtime_bridge.h"
#include "stormbolt_attachment_update.h"
#include "stormbolt_middle_native_fields.h"
#include "stormbolt_power_cell_visual_color.h"
#include "stormbolt_power_cell_update.h"
#include "stormbolt_power_cell_event_context.h"
#include "stormbolt_damage_exception_store.h"
#include "stormbolt_power_cell_growth_gate.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace stormslinger;
namespace {
void* visual_material=nullptr;
std::uint32_t visual_hash=0;
std::array<float,4> visual_values{};
unsigned visual_writes=0;
std::array<float,4> startup_rgba{};
void capture_startup(void*,std::uint32_t hash,const float* value) {
    assert(hash==stormbolt_emissive_parameter_hash);
    std::memcpy(startup_rgba.data(),value,sizeof(startup_rgba));
}
std::vector<std::uint32_t> visual_order;
float visual_scalar=0;
void capture_scalar(void* material,std::uint32_t hash,float value) {
    visual_material=material;visual_hash=hash;visual_scalar=value;
    visual_order.push_back(hash);
}
void capture_visual(void* material,std::uint32_t hash,const float* values) {
    visual_material=material;visual_hash=hash;++visual_writes;
    visual_order.push_back(hash);
    std::memcpy(visual_values.data(),values,sizeof(visual_values));
}
struct ProviderFaultMemory {
    inline static unsigned reads = 0, fail_at = 0;
    template<class T> static bool read(std::uintptr_t base, std::size_t offset, T& output) noexcept {
        if (++reads == fail_at) return false;
        return read_stormbolt_field(base,offset,output);
    }
};
struct ProviderChangingMemory {
    inline static std::uintptr_t target = 0;
    inline static unsigned reads = 0;
    template<class T> static bool read(std::uintptr_t base, std::size_t offset, T& output) noexcept {
        if (!read_stormbolt_field(base,offset,output)) return false;
        if (base == target && offset == 0 && ++reads == 2) output ^= 8;
        return true;
    }
};
void same_bindings(const StormboltNativeBindings& one, const StormboltNativeBindings& two) {
    assert(one.evade_component_vtable==two.evade_component_vtable);
    assert(one.evade_refill_call==two.evade_refill_call && one.evade_refill_expected==two.evade_refill_expected &&
        one.evade_effect.original_refill==two.evade_effect.original_refill && one.evade_effect.weapon_effect==two.evade_effect.weapon_effect &&
        one.evade_effect.ready==two.evade_effect.ready && one.evade_effect.context==two.evade_effect.context &&
        one.evade_effect.resolved_model_offset==two.evade_effect.resolved_model_offset);
    assert(one.source_mine_enabled==two.source_mine_enabled);
    assert(one.source_command==two.source_command);
    assert(one.source_platform_blocked==two.source_platform_blocked);
    assert(one.source_command_system_slot==two.source_command_system_slot);
    assert(one.source_manager_vtable==two.source_manager_vtable &&
        one.source_manager.initialize==two.source_manager.initialize &&
        one.source_manager.rebuild==two.source_manager.rebuild &&
        one.source_manager.tick==two.source_manager.tick &&
        one.source_manager.destroy==two.source_manager.destroy);
    assert(one.shoot_condition.type==two.shoot_condition.type &&
        one.shoot_condition.create==two.shoot_condition.create &&
        one.shoot_condition.initialize==two.shoot_condition.initialize &&
        one.shoot_condition.destroy==two.shoot_condition.destroy &&
        one.shoot_condition.vtable==two.shoot_condition.vtable &&
        one.shoot_condition.actor_vtable==two.shoot_condition.actor_vtable &&
        one.shoot_condition.weapon_vtable==two.shoot_condition.weapon_vtable &&
        one.shoot_condition.ready==two.shoot_condition.ready &&
        one.shoot_condition.ready_context==two.shoot_condition.ready_context);
    assert(one.source_transition_condition.transition_vtable==two.source_transition_condition.transition_vtable &&
        one.source_transition_condition.actor_vtable==two.source_transition_condition.actor_vtable &&
        one.source_transition_condition.weapon_vtable==two.source_transition_condition.weapon_vtable &&
        one.source_transition_condition.original==two.source_transition_condition.original &&
        one.source_transition_condition.ready==two.source_transition_condition.ready &&
        one.source_transition_condition.ready_with_context==two.source_transition_condition.ready_with_context &&
        one.source_transition_condition.ready_context==two.source_transition_condition.ready_context);
    assert(one.source_transition_type==two.source_transition_type && one.source_transition_heap==two.source_transition_heap);
    assert(one.source_transition.type_vtable==two.source_transition.type_vtable &&
        one.source_transition.original_create==two.source_transition.original_create &&
        one.source_transition.original_place==two.source_transition.original_place &&
        one.source_transition.original_destroy==two.source_transition.original_destroy);
    assert(one.power_cell_visual_convert==two.power_cell_visual_convert);
    assert(one.power_cell_material_at==two.power_cell_material_at);
    assert(one.power_cell_material_scalar==two.power_cell_material_scalar);
    assert(one.power_cell_event_actor==two.power_cell_event_actor);
    assert(one.power_cell_timer_update_site==two.power_cell_timer_update_site &&
        one.power_cell_timer_native_query==two.power_cell_timer_native_query);
    assert(one.damage_construct==two.damage_construct && one.damage_destroy==two.damage_destroy &&
        one.damage_update==two.damage_update && one.damage_absorb==two.damage_absorb && one.damage_vtable==two.damage_vtable);
    assert(one.power_cell_remote_update_site==two.power_cell_remote_update_site);
    assert(one.class48_skill_multiplier==two.class48_skill_multiplier);
    assert(one.power_cell_growth_sites==two.power_cell_growth_sites);
    assert(one.power_cell_sync_pack_site==two.power_cell_sync_pack_site &&
        one.power_cell_sync_complete_site==two.power_cell_sync_complete_site);
    assert(one.power_cell_sync_construct==two.power_cell_sync_construct &&
        one.power_cell_sync_destroy==two.power_cell_sync_destroy && one.power_cell_sync_vtable==two.power_cell_sync_vtable);
    assert(one.power_cell_status_update_site==two.power_cell_status_update_site &&
        one.power_cell_status_manager_slot==two.power_cell_status_manager_slot);
    assert(one.local_input.query==two.local_input.query);
    assert(one.update_gate.authority_slot==two.update_gate.authority_slot && one.update_gate.query==two.update_gate.query);
    assert(one.action_descriptor==two.action_descriptor);
    assert(one.remote_attachment==two.remote_attachment);
    assert(one.health_adjustment==two.health_adjustment);
    assert(one.equipment_effect==two.equipment_effect);
    assert(one.skill_value_query==two.skill_value_query);
    assert(one.skill_prompt==two.skill_prompt);
    assert(one.loop_construct==two.loop_construct && one.loop_destroy==two.loop_destroy &&
           one.loop_update_site==two.loop_update_site &&
           one.loop_default_first==two.loop_default_first && one.loop_default_second==two.loop_default_second &&
           one.loop_vtable==two.loop_vtable && one.named_actor_event==two.named_actor_event &&
           one.sound_container_at==two.sound_container_at);
    assert(one.power_cell.construct==two.power_cell.construct && one.power_cell.reset==two.power_cell.reset &&
        one.power_cell.destroy==two.power_cell.destroy && one.power_cell.status_vtable==two.power_cell.status_vtable &&
        one.power_cell.resource_slot==two.power_cell.resource_slot && one.power_cell.resource_vtable==two.power_cell.resource_vtable);
    assert(one.weapon_update.weapon_vtable==two.weapon_update.weapon_vtable &&
        one.weapon_update.original==two.weapon_update.original && one.weapon_update.ready==two.weapon_update.ready);
#define SAME_STAGE(field) assert(one.stage_update.field==two.stage_update.field)
    SAME_STAGE(gate.authority_slot);SAME_STAGE(gate.query);SAME_STAGE(descriptor);SAME_STAGE(ammo_items);
    SAME_STAGE(routes.input.query);SAME_STAGE(routes.trigger.primary);SAME_STAGE(routes.trigger.alternate_gate);
    SAME_STAGE(routes.attachment);SAME_STAGE(routes.effect.active);SAME_STAGE(routes.effect.request);SAME_STAGE(routes.effect.assign);
    SAME_STAGE(routes.cleanup.active);SAME_STAGE(routes.cleanup.stop);SAME_STAGE(routes.cleanup.assign);
    SAME_STAGE(routes.owner.manager_slot);SAME_STAGE(routes.owner.local_player);SAME_STAGE(routes.actor_flag);
    SAME_STAGE(routes.color.active);SAME_STAGE(routes.color.set_color);SAME_STAGE(routes.material.write_vector);
#undef SAME_STAGE
    assert(one.input_trigger.primary==two.input_trigger.primary && one.input_trigger.alternate_gate==two.input_trigger.alternate_gate);
    assert(one.material_color.write_vector==two.material_color.write_vector);
    assert(one.effect_color.active==two.effect_color.active && one.effect_color.set_color==two.effect_color.set_color);
    assert(one.decay_owner.manager_slot==two.decay_owner.manager_slot && one.decay_owner.local_player==two.decay_owner.local_player);
    assert(one.decay_effect.active==two.decay_effect.active && one.decay_effect.request==two.decay_effect.request &&
        one.decay_effect.assign==two.decay_effect.assign);
    assert(one.cleanup_bank.active==two.cleanup_bank.active && one.cleanup_bank.stop==two.cleanup_bank.stop &&
        one.cleanup_bank.assign==two.cleanup_bank.assign);
    assert(one.last_update.event_object==two.last_update.event_object && one.last_update.difference==two.last_update.difference &&
        one.last_update.origin==two.last_update.origin && one.last_update.fire==two.last_update.fire &&
        one.last_update.actor_flag==two.last_update.actor_flag && one.last_update.effect_event==two.last_update.effect_event &&
        one.last_update.complete==two.last_update.complete && one.last_update.effect_context==two.last_update.effect_context &&
        one.last_update.effect_with_context==two.last_update.effect_with_context);
    assert(one.last_hook.action_vtable==two.last_hook.action_vtable && one.last_hook.actor_vtable==two.last_hook.actor_vtable &&
        one.last_hook.weapon_vtable==two.last_hook.weapon_vtable && one.last_hook.original_enter==two.last_hook.original_enter &&
        one.last_hook.original_exit==two.last_hook.original_exit && one.last_hook.original_update==two.last_hook.original_update &&
        one.last_hook.ready==two.last_hook.ready);
    assert(one.last_entry.mode==two.last_entry.mode && one.last_entry.parameter==two.last_entry.parameter &&
        one.last_entry.request_motion==two.last_entry.request_motion);
    assert(one.middle_entry.mode==two.middle_entry.mode && one.middle_entry.parameter==two.middle_entry.parameter &&
        one.middle_entry.needs_parameter==two.middle_entry.needs_parameter &&
        one.middle_entry.read_stage==two.middle_entry.read_stage && one.middle_entry.write_stage==two.middle_entry.write_stage);
    assert(one.middle_hook.action_vtable==two.middle_hook.action_vtable &&
        one.middle_hook.actor_vtable==two.middle_hook.actor_vtable && one.middle_hook.weapon_vtable==two.middle_hook.weapon_vtable &&
        one.middle_hook.original_enter==two.middle_hook.original_enter && one.middle_hook.original_update==two.middle_hook.original_update &&
        one.middle_hook.original_exit==two.middle_hook.original_exit && one.middle_hook.ready==two.middle_hook.ready);
    assert(one.middle_prepare.difference==two.middle_prepare.difference && one.middle_prepare.rate==two.middle_prepare.rate);
    assert(one.middle_prepare.animation.motion==two.middle_prepare.animation.motion &&
        one.middle_prepare.animation.actor_flag==two.middle_prepare.animation.actor_flag &&
        one.middle_prepare.animation.event_object==two.middle_prepare.animation.event_object &&
        one.middle_prepare.animation.weapon_effect==two.middle_prepare.animation.weapon_effect &&
        one.middle_prepare.animation.query==two.middle_prepare.animation.query);
    assert(one.middle_fire.event_object==two.middle_fire.event_object &&
        one.middle_fire.origin==two.middle_fire.origin && one.middle_fire.fire==two.middle_fire.fire);
    assert(one.gatling_hook.action_vtable==two.gatling_hook.action_vtable &&
        one.gatling_hook.actor_vtable==two.gatling_hook.actor_vtable &&
        one.gatling_hook.weapon_vtable==two.gatling_hook.weapon_vtable &&
        one.gatling_hook.original_enter==two.gatling_hook.original_enter &&
        one.gatling_hook.original_update==two.gatling_hook.original_update &&
        one.gatling_hook.original_exit==two.gatling_hook.original_exit &&
        one.gatling_hook.ready==two.gatling_hook.ready);
    assert(one.gatling_entry.player_manager_slot == two.gatling_entry.player_manager_slot &&
        one.gatling_entry.local_player == two.gatling_entry.local_player &&
        one.gatling_entry.ammo_items == two.gatling_entry.ammo_items &&
        one.gatling_entry.animation.motion == two.gatling_entry.animation.motion &&
        one.gatling_entry.animation.actor_flag == two.gatling_entry.animation.actor_flag &&
        one.gatling_entry.animation.event_object == two.gatling_entry.animation.event_object &&
        one.gatling_entry.animation.weapon_effect == two.gatling_entry.animation.weapon_effect &&
        one.gatling_entry.animation.query == two.gatling_entry.animation.query);
    assert(one.lifecycle.allocate == two.lifecycle.allocate && one.lifecycle.place == two.lifecycle.place &&
        one.lifecycle.destroy == two.lifecycle.destroy);
    assert(one.shell.create_entry == two.shell.create_entry && one.shell.caller_return == two.shell.caller_return &&
        one.shell.weapon_vtable == two.shell.weapon_vtable && one.shell.list_vtable == two.shell.list_vtable &&
        one.shell.record_at == two.shell.record_at);
    assert(one.coefficients.slot == two.coefficients.slot && one.coefficients.original == two.coefficients.original &&
        one.coefficients.caller_return == two.coefficients.caller_return &&
        one.coefficients.weapon_vtable == two.coefficients.weapon_vtable &&
        one.coefficients.info_vtable == two.coefficients.info_vtable &&
        one.coefficients.parameter_vtable == two.coefficients.parameter_vtable);
    assert(one.normal_to_special.slot == two.normal_to_special.slot &&
        one.normal_to_special.original == two.normal_to_special.original &&
        one.normal_to_special.action_vtable == two.normal_to_special.action_vtable &&
        one.normal_to_special.actor_vtable == two.normal_to_special.actor_vtable &&
        one.normal_to_special.weapon_vtable == two.normal_to_special.weapon_vtable &&
        one.normal_to_special.event_object == two.normal_to_special.event_object &&
        one.normal_to_special.query == two.normal_to_special.query);
#define SAME_SPECIAL(field) assert(one.special_to_normal.field == two.special_to_normal.field)
    SAME_SPECIAL(hook.action_vtable); SAME_SPECIAL(hook.actor_vtable); SAME_SPECIAL(hook.weapon_vtable);
    SAME_SPECIAL(hook.original_enter); SAME_SPECIAL(hook.original_update); SAME_SPECIAL(hook.original_exit);
    SAME_SPECIAL(hook.ready);
    SAME_SPECIAL(action.motion); SAME_SPECIAL(action.actor_flag); SAME_SPECIAL(action.event_object);
    SAME_SPECIAL(action.weapon_effect); SAME_SPECIAL(action.query);
    SAME_SPECIAL(save.manager_slot); SAME_SPECIAL(save.current_character); SAME_SPECIAL(save.set_character_bit);
    SAME_SPECIAL(effects.parameter_dti); SAME_SPECIAL(effects.construct_parameter); SAME_SPECIAL(effects.actor_selector);
    SAME_SPECIAL(effects.provider_slot); SAME_SPECIAL(effects.provider_context); SAME_SPECIAL(effects.create);
    SAME_SPECIAL(effects.post_needed); SAME_SPECIAL(effects.post_manager_slot); SAME_SPECIAL(effects.post);
    SAME_SPECIAL(effects.bank_active); SAME_SPECIAL(effects.assign_bank); SAME_SPECIAL(effects.clear_bank70);
    SAME_SPECIAL(effects.provider_resolver_context); SAME_SPECIAL(effects.resolve_provider);
    SAME_SPECIAL(provider_identity.singleton_slot); SAME_SPECIAL(provider_identity.vtable);
    SAME_SPECIAL(provider_identity.dti); SAME_SPECIAL(provider_identity.dti_vtable);
    SAME_SPECIAL(provider_identity.dti_getter); SAME_SPECIAL(provider_identity.record_create);
    SAME_SPECIAL(provider_identity.create);
#undef SAME_SPECIAL
}
}
int main() {
    {
        // Real DTI-selected alternate profile; only allocator IO is a fixture.
        alignas(16) std::array<unsigned char,0x650> heap{};
        std::array<std::uintptr_t,14> table{};std::array<void*,64> slots{};
        std::array<unsigned char,0x40> type{};
        auto allocate=+[](void* h,std::size_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,const void*)->void* {return h;};
        table[7]=reinterpret_cast<std::uintptr_t>(allocate);table[13]=0x1234;
        const auto vt=reinterpret_cast<std::uintptr_t>(table.data());
        std::memcpy(heap.data(),&vt,sizeof(vt));slots[12]=heap.data();
        const std::uint32_t packed=12u<<23;std::memcpy(type.data()+0x30,&packed,4);
        StormboltDefaultHeapBindings api{reinterpret_cast<std::uintptr_t>(slots.data()),1,2,3,vt,table[7],table[13]};
        StormboltDefaultHeapSelection selected{};
        assert(stormbolt_select_default_heap(api,type.data(),selected) && selected.index==12 && selected.allocate==table[7]);
        assert(stormbolt_allocate_default_heap(api,type.data(),heap.data(),48,16,0,0,0,nullptr)==heap.data());
        table[7]^=1;
        assert(!stormbolt_select_default_heap(api,type.data(),selected));
        assert(!stormbolt_allocate_default_heap(api,type.data(),heap.data(),48,16,0,0,0,nullptr));
        table[7]^=1;api.typed_release^=1;
        assert(!stormbolt_select_default_heap(api,type.data(),selected));
    }

    {
        int quest_calls=0,shooting_calls=0;
        auto quest=[&] {++quest_calls;return 0;};
        auto shooting=[&](const StormboltRuntimeReadiness& readiness) {
            assert(quest_calls==1);
            assert(readiness.conditions.synchronous_borrow_verified && readiness.evade.synchronous_borrow_verified);
            assert(readiness.color==0xFFFF8214u && !readiness.evade.resource_ready);
            ++shooting_calls;return -271;
        };
        assert(stormbolt_start_native_callback_candidate(quest,shooting)==-271);
        assert(quest_calls==1 && shooting_calls==1);
        assert(stormbolt_start_native_callback_candidate([] {return -13;},shooting)==-13);
        assert(shooting_calls==1);
        assert(stormbolt_start_native_callback_candidate([] {return 0;},[](const StormboltRuntimeReadiness&) {return 0;})==0);
        StormboltRuntimeReadiness strict;
        assert(!strict.conditions.synchronous_borrow_verified && !strict.evade.synchronous_borrow_verified);
    }

    {
        // Actual startup defaults must reach stage selection and material write,
        // including the original counter5/6 boundary and caller overrides.
        StormboltRuntimeReadiness configuration;
        assert(configuration.color==0xFFFF8214u);
        assert(!configuration.conditions.synchronous_borrow_verified && !configuration.evade.synchronous_borrow_verified);
        StormboltInstanceStore<1> instances;
        std::array<unsigned char,0x70> weapon{};int material=0;
        assert(instances.initialize(reinterpret_cast<std::uintptr_t>(weapon.data())));
        StormboltInstanceToken token{};
        assert(instances.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon.data()),token));
        assert(instances.transact(token,[](auto& state) noexcept {state.counter=5;return true;}));
        assert(stormbolt_update_attachment_color(instances,token,&material,configuration.low,configuration.high,{&capture_startup}));
        assert(std::fabs(startup_rgba[0])<0.001f && std::fabs(startup_rgba[1]-10.54f)<0.001f && startup_rgba[2]==20.0f && startup_rgba[3]==1.0f);
        assert(instances.transact(token,[](auto& state) noexcept {state.counter=6;return true;}));
        assert(stormbolt_update_attachment_color(instances,token,&material,configuration.low,configuration.high,{&capture_startup}));
        assert(std::fabs(startup_rgba[0]-68.0f)<0.001f && std::fabs(startup_rgba[1]-20.0f)<0.001f && startup_rgba[2]==100.0f && startup_rgba[3]==1.0f);
        configuration.low={0,0.25f,0,0.7f};
        assert(instances.transact(token,[](auto& state) noexcept {state.counter=5;return true;}));
        assert(stormbolt_update_attachment_color(instances,token,&material,configuration.low,configuration.high,{&capture_startup}));
        assert(startup_rgba[0]==5.0f && startup_rgba[1]==5.0f && startup_rgba[2]==5.0f && startup_rgba[3]==0.7f);
    }
    // Execute only our field accessors against owned storage, never game code.
    std::array<unsigned char,0x13E80> actor_fields{};
    actor_fields.fill(0xA5);
    for (std::uint32_t value : {0u,1u,0x100u,0x10000u,0x1000000u,0xffffffffu}) {
        std::memcpy(actor_fields.data()+0x1128,&value,sizeof(value));
        assert(stormbolt_middle_needs_parameter(actor_fields.data())==(value==0));
    }
    for (unsigned value=0; value<256; ++value) {
        const auto before=actor_fields;
        stormbolt_middle_write_stage(actor_fields.data(),static_cast<std::uint8_t>(value));
        assert(stormbolt_middle_read_stage(actor_fields.data())==value);
        for (std::size_t i=0;i<actor_fields.size();++i)
            if (i!=0x13E7E) assert(actor_fields[i]==before[i]);
    }

    using namespace native_shooting_guard;
    // Offline image fixture. Only the standalone color conversion page is
    // temporarily executable; no game process or hooks are involved.
    auto* memory = static_cast<unsigned char*>(VirtualAlloc(nullptr, image_size,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    assert(memory);
    const auto base = reinterpret_cast<std::uintptr_t>(memory);
    for (const auto& span : spans) std::memcpy(memory + span.rva, span.bytes, span.size);
    std::memcpy(memory+ammo_items_rva, ammo_items_data, sizeof(ammo_items_data));
    std::memcpy(memory+trigger_map_rva,trigger_map_data,sizeof(trigger_map_data));
    for (const auto& pointer : pointers) {
        const auto value = base + pointer.value_rva;
        std::memcpy(memory + pointer.slot_rva, &value, sizeof(value));
    }
    StormboltNativeBindings bindings{};
    assert(bind_stormbolt_native_code(base, image_size, bindings));
    {
        StormboltRuntimeReadiness request;
        const auto configured=stormbolt_configure_runtime_readiness(bindings,request);
        const auto condition=stormbolt_condition_readiness_bindings(bindings);
        const auto evade=stormbolt_evade_effect_readiness_bindings(bindings);
        assert(configured.conditions.actor_vtable==condition.actor_vtable);
        assert(configured.conditions.weapon_vtable==condition.weapon_vtable);
        assert(configured.conditions.parameter_vtable==condition.parameter_vtable);
        assert(configured.conditions.manager_vtable==condition.manager_vtable);
        assert(configured.conditions.authority_slot==condition.authority_slot);
        assert(configured.conditions.command_system_slot==condition.command_system_slot);
        assert(configured.conditions.player_manager_slot==condition.player_manager_slot);
        assert(configured.evade.weapon_vtable==evade.weapon_vtable && configured.evade.actor_vtable==evade.actor_vtable);
        assert(configured.evade.component_vtable==evade.component_vtable && configured.evade.mesh==evade.mesh);
        assert(configured.evade.provider.singleton_slot==evade.provider.singleton_slot);
        assert(configured.evade.provider.create==evade.provider.create);
        assert(configured.evade.epv_resource_vtable==evade.epv_resource_vtable && configured.evade.efx_resource_vtable==evade.efx_resource_vtable);
        assert(reinterpret_cast<std::uintptr_t>(configured.allocate)==bindings.default_heap.allocate);
        assert(!configured.conditions.synchronous_borrow_verified && !configured.evade.synchronous_borrow_verified);
        assert(!configured.evade.resource_ready && !configured.evade.resource_context);
        // Do not erase an explicit wrong identity: downstream validation must
        // still reject it. Preserve host callbacks, contexts and color overrides.
        request.conditions.actor_vtable=0xDEAD;
        request.evade.mesh.model=0xBEEF;
        request.evade.resource_context=&request;
        request.color=0x12345678;request.low={1,2,3,4};
        request.evade.resource_ready=+[](void*,void*,std::uint32_t,std::uint32_t,void*) {return false;};
        const auto explicit_request=stormbolt_configure_runtime_readiness(bindings,request);
        assert(explicit_request.conditions.actor_vtable==0xDEAD && explicit_request.evade.mesh.model==0xBEEF);
        assert(explicit_request.evade.resource_context==&request && explicit_request.evade.resource_ready==request.evade.resource_ready);
        assert(explicit_request.color==request.color && explicit_request.low==request.low);
        assert(!request.conditions.weapon_vtable && !request.allocate);
    }

    assert((bindings.evade_refill_call==base+0x187A2AE && bindings.evade_refill_expected==std::array<unsigned char,5>({0xE8,0xFD,0xE5,0x8C,0x00})));
    assert(std::memcmp(reinterpret_cast<void*>(bindings.evade_refill_call),bindings.evade_refill_expected.data(),5)==0);
    const auto evade_ready_config=stormbolt_evade_effect_readiness_bindings(bindings);
    assert(bindings.evade_epv_resource_vtable==base+0x3518FD8 &&
        evade_ready_config.epv_resource_vtable==bindings.evade_epv_resource_vtable);
    assert(bindings.evade_efx_resource_vtable==base+0x3525848 &&
        evade_ready_config.efx_resource_vtable==bindings.evade_efx_resource_vtable);
    assert(bindings.evade_mesh==evade_ready_config.mesh && bindings.evade_mesh.complete());
    assert(bindings.evade_mesh.container==base+0x3551948 && bindings.evade_mesh.emitter==base+0x357CF90 &&
        bindings.evade_mesh.parameter==base+0x351FC98 && bindings.evade_mesh.model==base+0x3507A18 &&
        bindings.evade_mesh.positions==base+0x357D260 && bindings.evade_mesh.material==base+0x351A078 &&
        bindings.evade_mesh.material_manager_slot==base+0x51C73D0 && bindings.evade_mesh.texture==base+0x3512160);
    assert(bindings.evade_mesh.group==base+0x357D008 && bindings.evade_mesh.play_efx==base+0x35899C8);
    assert(bindings.evade_component_vtable==base+0x3118D20 && evade_ready_config.component_vtable==bindings.evade_component_vtable &&
        evade_ready_config.actor_vtable==bindings.normal_to_special.actor_vtable &&
        evade_ready_config.weapon_vtable==bindings.shell.weapon_vtable && !evade_ready_config.synchronous_borrow_verified &&
        !evade_ready_config.resource_ready && evade_ready_config.provider.singleton_slot==bindings.special_to_normal.provider_identity.singleton_slot);
    assert(reinterpret_cast<std::uintptr_t>(bindings.evade_effect.original_refill)==base+0x21488B0 &&
        reinterpret_cast<std::uintptr_t>(bindings.evade_effect.weapon_effect)==base+0xAE2910 &&
        !bindings.evade_effect.ready && bindings.evade_effect.resolved_model_offset==0x1D00);
    assert(bindings.power_cell_status_update_site==base+0x11C6CE3 &&
        bindings.power_cell_status_manager_slot==base+0x500ECA0);
    assert(bindings.power_cell_timer_update_site==base+0x11C7F18 &&
        reinterpret_cast<std::uintptr_t>(bindings.power_cell_timer_native_query)==base+0x1F628A0);
    assert(bindings.damage_construct==base+0x1216EF0 && bindings.damage_destroy==base+0x1217340 &&
        bindings.damage_update==base+0x121B8D0 && bindings.damage_absorb==base+0x118EFE0 &&
        bindings.damage_vtable==base+0x3240FB0);
    assert(bindings.power_cell_remote_update_site==base+0x11B034A);
    assert(bindings.class48_skill_multiplier==base+0x11BC1D0);
    assert(bindings.source_transition_type==reinterpret_cast<void*>(base+0x4F7E7D0));
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_transition_heap)==base+0x216F3F0);
    assert(bindings.source_transition.type_vtable==base+0x31C4A40);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_mine_enabled)==base+0x2147A80);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_command)==base+0x17BE130);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_platform_blocked)==base+0x240720);
    const auto fields=stormbolt_source_transition_field_api(bindings);
    const auto shoot_fields=stormbolt_shoot_gatling_field_api(bindings);
    const auto shoot_object=stormbolt_shoot_gatling_condition_api(bindings);
    const auto manager_api=stormbolt_source_manager_api(bindings);
    const auto readiness=stormbolt_condition_readiness_bindings(bindings);
    assert(!readiness.synchronous_borrow_verified && readiness.command_system_slot==bindings.source_command_system_slot &&
        reinterpret_cast<std::uintptr_t>(readiness.command_system_slot)==base+0x51C4640 &&
        readiness.actor_vtable==bindings.normal_to_special.actor_vtable && readiness.weapon_vtable==bindings.shell.weapon_vtable &&
        readiness.parameter_vtable==bindings.coefficients.parameter_vtable && readiness.manager_vtable==bindings.source_manager_vtable &&
        readiness.authority_slot==bindings.update_gate.authority_slot && readiness.player_manager_slot==bindings.gatling_entry.player_manager_slot);
    assert(manager_api.manager_vtable==base+0x322BBB0 && !manager_api.condition.ready);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_manager.initialize)==base+0x118DCA0 &&
        reinterpret_cast<std::uintptr_t>(bindings.source_manager.rebuild)==base+0x118DDC0 &&
        reinterpret_cast<std::uintptr_t>(bindings.source_manager.tick)==base+0x118DBB0 &&
        reinterpret_cast<std::uintptr_t>(bindings.source_manager.destroy)==base+0x118E810);
    assert(shoot_object.type==reinterpret_cast<void*>(base+0x4F9F240) &&
        reinterpret_cast<std::uintptr_t>(shoot_object.create)==base+0x11CB370 &&
        reinterpret_cast<std::uintptr_t>(shoot_object.initialize)==base+0x11CB480 &&
        reinterpret_cast<std::uintptr_t>(shoot_object.destroy)==base+0xEF3CA0 &&
        shoot_object.vtable==reinterpret_cast<void**>(base+0x31C0AA0) &&
        shoot_object.actor_vtable==base+0x34A6D18 && shoot_object.weapon_vtable==base+0x34D6C88 &&
        !shoot_object.ready && !shoot_object.ready_context);
    assert(shoot_object.fields.held_query==shoot_fields.held_query &&
        shoot_object.fields.physical_primary==shoot_fields.physical_primary &&
        shoot_object.fields.parameter_vtable==shoot_fields.parameter_vtable);
    assert(shoot_fields.held_query==bindings.local_input.query &&
        shoot_fields.physical_primary==bindings.input_trigger.primary &&
        shoot_fields.parameter_vtable==bindings.coefficients.parameter_vtable);
    assert(fields.remaining==&stormbolt_source_remaining && fields.direct_toggle==&stormbolt_source_direct_toggle && fields.source_count==&stormbolt_source_count &&
        fields.special_mode==&stormbolt_source_special_mode && fields.charge_ratio==&stormbolt_source_charge_ratio &&
        fields.mine_enabled==bindings.source_mine_enabled && fields.command==bindings.source_command && fields.complete());
    assert(fields.native_platform_blocked==bindings.source_platform_blocked &&
        fields.native_authority_slot==bindings.update_gate.authority_slot &&
        fields.native_authority_query==bindings.update_gate.query);
    assert(bindings.source_transition_condition.transition_vtable==base+0x31C92B0 &&
        bindings.source_transition_condition.actor_vtable==base+0x34A6D18 &&
        bindings.source_transition_condition.weapon_vtable==base+0x34D6C88 &&
        reinterpret_cast<std::uintptr_t>(bindings.source_transition_condition.original)==base+0xF13060 &&
        !bindings.source_transition_condition.ready);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_transition.original_create)==base+0xF03CA0);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_transition.original_place)==base+0xF03D20);
    assert(reinterpret_cast<std::uintptr_t>(bindings.source_transition.original_destroy)==base+0xF0AE50);
    const auto source_api=stormbolt_source_transition_create_api(bindings,nullptr);
    assert(source_api.type==bindings.source_transition_type && source_api.allocator_for==bindings.source_transition_heap &&
        source_api.place==bindings.source_transition.original_place && !source_api.allocate);
    const auto class48=stormbolt_class48_skill_bindings(bindings);
    assert(class48.status_vtable==bindings.power_cell.status_vtable && class48.resource_slot==bindings.loop_inputs.resource_slot && class48.resource_vtable==bindings.loop_inputs.resource_vtable && class48.query==bindings.skill_value_query);
    assert((bindings.power_cell_growth_sites==std::array<std::uintptr_t,3>{{base+0x121B37F,base+0x121B39E,base+0x121B3A7}}));
    assert(bindings.power_cell_sync_pack_site==base+0x1199E99 &&
        bindings.power_cell_sync_complete_site==base+0x1199F4B);
    assert(bindings.power_cell_sync_construct==base+0x11927A0 &&
        bindings.power_cell_sync_destroy==base+0x1193610 && bindings.power_cell_sync_vtable==base+0x322BBB0);
    const auto pack_factory=stormbolt_sync_pack_bindings(bindings);
    const auto remote_factory=stormbolt_remote_update_bindings(bindings);
    const auto growth_factory=stormbolt_complete_growth_bindings(bindings);
    assert(pack_factory.master_vtable==bindings.power_cell_sync_vtable &&
        pack_factory.query==bindings.skill_value_query && pack_factory.encode==&encode_stormbolt_power_cell_sync);
    assert(remote_factory.decode==&decode_stormbolt_power_cell_sync &&
        remote_factory.manager_slot==bindings.gatling_entry.player_manager_slot &&
        remote_factory.local_player==bindings.gatling_entry.local_player && remote_factory.equipment==bindings.equipment_effect);
    assert(growth_factory.growth.damage_vtable==bindings.damage_vtable &&
        growth_factory.master_vtable==bindings.power_cell_sync_vtable && growth_factory.equipment==bindings.equipment_effect &&
        growth_factory.growth.prompt==bindings.skill_prompt && growth_factory.growth.local_player==bindings.gatling_entry.local_player);
    const auto component_bindings=stormbolt_loop_component_bindings(bindings);
    const auto status_bindings=stormbolt_status_update_bindings(bindings);
    assert(status_bindings.manager_slot==bindings.power_cell_status_manager_slot &&
        status_bindings.query==bindings.skill_value_query &&
        status_bindings.actor_flag==bindings.special_to_normal.action.actor_flag);
    assert(component_bindings.audio.component_vtable==bindings.loop_vtable &&
        component_bindings.audio.query==bindings.skill_value_query &&
        component_bindings.audio.request==bindings.named_actor_event &&
        component_bindings.material_at==bindings.power_cell_material_at &&
        component_bindings.convert==bindings.power_cell_visual_convert &&
        component_bindings.scalar==bindings.power_cell_material_scalar &&
        component_bindings.material.write_vector==bindings.material_color.write_vector);
    assert(reinterpret_cast<std::uintptr_t>(bindings.power_cell_visual_convert)==base+0x2171490);
    assert(reinterpret_cast<std::uintptr_t>(bindings.power_cell_material_at)==base+0x222F910);
    {
        std::array<std::byte,0x89A0> flag_owner{};
        std::uint32_t original_primary=0x80,original_shadow=0x20;
        std::memcpy(flag_owner.data()+0x8948,&original_primary,4);
        std::memcpy(flag_owner.data()+0x8978,&original_shadow,4);
        DWORD previous=0,ignored=0;
        assert(VirtualProtect(memory+0x1F60000,0x1000,PAGE_EXECUTE_READ,&previous));
        assert(FlushInstructionCache(GetCurrentProcess(),memory+0x1F604B0,0x5D));
        bindings.special_to_normal.action.actor_flag(flag_owner.data(),0x98);
        std::uint32_t primary=0,shadow=0;
        std::memcpy(&primary,flag_owner.data()+0x8948,4);
        std::memcpy(&shadow,flag_owner.data()+0x8978,4);
        assert(primary==0x01000080u && shadow==0x01000020u);
        assert(VirtualProtect(memory+0x1F60000,0x1000,previous,&ignored));
    }
    {
        std::vector<std::byte> manager(0xAE48);
        std::array<std::byte,0x20> local_actor{};
        const auto put=[](auto& bytes,std::size_t offset,const auto& value) {
            std::memcpy(bytes.data()+offset,&value,sizeof(value));
        };
        for (unsigned i=0;i<24;++i) {
            put(manager,0x50+i*0x740,std::int32_t{-1});
            put(manager,0x54+i*0x740,std::int32_t{-1});
        }
        int identity=0;
        put(manager,0xAE40,static_cast<void*>(&identity));
        put(manager,0x48+2*0x740,static_cast<void*>(&identity));
        put(manager,0x50+2*0x740,std::int32_t{0});
        put(manager,0x58+2*0x740,static_cast<void*>(local_actor.data()));
        local_actor[0xC]=std::byte{0xE};
        DWORD previous=0,ignored=0;
        assert(VirtualProtect(memory+0x1B42000,0x1000,PAGE_EXECUTE_READ,&previous));
        assert(FlushInstructionCache(GetCurrentProcess(),memory+0x1B42010,0x100));
        assert(bindings.gatling_entry.local_player(manager.data())==local_actor.data());
        local_actor[0xC]=std::byte{0};
        assert(!bindings.gatling_entry.local_player(manager.data()));
        local_actor[0xC]=std::byte{0xE};
        put(manager,0x50+2*0x740,std::int32_t{-1});
        assert(!bindings.gatling_entry.local_player(manager.data()));
        assert(VirtualProtect(memory+0x1B42000,0x1000,previous,&ignored));
    }
    {
        std::array<std::byte,0xC0> event{};
        std::array<std::byte,0xB50> source{};
        std::array<std::byte,0x20> associated{};
        const auto put=[](auto& b,std::size_t offset,const auto& v) {std::memcpy(b.data()+offset,&v,sizeof(v));};
        put(event,8,static_cast<void*>(source.data()));
        source[0xC]=associated[0xC]=std::byte{0xE};
        DWORD previous=0,ignored=0;
        assert(VirtualProtect(memory+0x12DB000,0x1000,PAGE_EXECUTE_READ,&previous));
        assert(FlushInstructionCache(GetCurrentProcess(),memory+0x12DBE50,sizeof(power_cell_event_actor_code)));
        assert(bindings.power_cell_event_actor(event.data())==source.data());
        std::array<std::byte,0x68> event_component{};
        std::array<std::byte,0x8940> event_owner{};
        std::array<std::byte,0x18> event_result{};
        put(event_component,0x60,static_cast<void*>(event_owner.data()));
        put(event_owner,0x8938,std::uint8_t{1});
        std::array<std::byte,0x290> damage_component{};
        std::array<std::byte,0xDB8> absorption_object{};
        put(damage_component,0,std::uintptr_t{55});
        put(event_owner,0x7658,static_cast<void*>(damage_component.data()));
        put(absorption_object,0xDB0,static_cast<void*>(event_owner.data()));
        StormboltDamageExceptionStore<2> exceptions;
        const auto exception_token=exceptions.initialize(reinterpret_cast<std::uintptr_t>(damage_component.data()));
        StormboltDamageExceptionState exception{};
        for (float input : {0.0f,-1.0f,std::numeric_limits<float>::quiet_NaN()}) {
            assert(mark_stormbolt_damage_exception_after_absorption(absorption_object.data(),input,55,exceptions));
            assert(exceptions.snapshot(exception_token,exception) && !exception.absorbed_positive_input);
        }
        assert(mark_stormbolt_damage_exception_after_absorption(absorption_object.data(),1.0f,55,exceptions));
        assert(mark_stormbolt_damage_exception_after_absorption(absorption_object.data(),0.0f,55,exceptions));
        assert(exceptions.snapshot(exception_token,exception) && exception.absorbed_positive_input);
        assert(!mark_stormbolt_damage_exception_after_absorption(absorption_object.data(),1.0f,56,exceptions));
        assert(clear_stormbolt_damage_exception_after_update(exception_token,exceptions));
        assert(exceptions.snapshot(exception_token,exception) && !exception.absorbed_positive_input);
        assert(exceptions.retire(exception_token));
        assert(!mark_stormbolt_damage_exception_after_absorption(absorption_object.data(),1.0f,55,exceptions));
        const auto replacement=exceptions.initialize(reinterpret_cast<std::uintptr_t>(damage_component.data()));
        assert(!clear_stormbolt_damage_exception_after_update(exception_token,exceptions));
        assert(!mark_stormbolt_damage_exception_for_captured_absorption(exception_token,1.0f,exceptions));
        assert(exceptions.snapshot(replacement,exception) && !exception.absorbed_positive_input);
        put(event_result,0,12.5f);put(event_result,0x10,std::uint32_t{7});
        put(source,0x60,std::uint64_t{1}<<14);
        StormboltPowerCellEventContext event_context{};
        std::array<std::byte,0x1A0> attack_metadata{};
        put(event,0xB8,static_cast<void*>(attack_metadata.data()));
        for (float scalar : {1.0f,0.0f,-1.0f}) {
            for (std::uint32_t code : {7u,0x27u,8u}) {
                put(attack_metadata,0x20,scalar);put(attack_metadata,0xA0,code);
                StormboltPowerCellEventMetadata metadata{};
                assert(read_stormbolt_power_cell_event_metadata(event.data(),metadata));
                assert(metadata.scalar==scalar && metadata.code==code);
                assert(stormbolt_power_cell_metadata_allows_growth(metadata)==
                    !(scalar<=0 && (code==7 || code==0x27)));
            }
        }
        const float unordered=std::numeric_limits<float>::quiet_NaN();
        assert(stormbolt_power_cell_metadata_allows_growth({unordered,7}));
        assert(!stormbolt_power_cell_metadata_allows_growth({-0.0f,7}));
        put(event,0xB8,static_cast<void*>(nullptr));
        StormboltPowerCellEventMetadata unchanged{12.0f,99};
        assert(!read_stormbolt_power_cell_event_metadata(event.data(),unchanged));
        assert(unchanged.scalar==12 && unchanged.code==99);
        StormboltPowerCellGrowthGateInputs gate{true,1.0f,false,true,false,{0,8},true,1,4,true,true};
        assert(stormbolt_power_cell_growth_gate(gate));
        gate.metadata={0,7};assert(!stormbolt_power_cell_growth_gate(gate));
        gate.metadata={0,8};gate.associated_actor_powered=false;
        assert(!stormbolt_power_cell_growth_gate(gate));
        gate.event_eligible=true;assert(stormbolt_power_cell_growth_gate(gate));
        gate.amount=unordered;assert(!stormbolt_power_cell_growth_gate(gate));
        gate.absorbed_positive_input=true;assert(stormbolt_power_cell_growth_gate(gate));
        gate.count=4;assert(!stormbolt_power_cell_growth_gate(gate));
        gate.count=5;assert(!stormbolt_power_cell_growth_gate(gate));
        assert(decide_stormbolt_power_cell_growth(gate).continue_effects);
        gate.count=1;gate.associated_actor_is_local=false;assert(!stormbolt_power_cell_growth_gate(gate));
        assert(decide_stormbolt_power_cell_growth(gate).continue_effects);
        gate.metadata={0,7};assert(!decide_stormbolt_power_cell_growth(gate).continue_effects);
        assert(read_stormbolt_power_cell_event_context(event_component.data(),event.data(),event_result.data(),
            bindings.power_cell_event_actor,event_context));
        assert(event_context.owner==event_owner.data() && event_context.event_actor==source.data() &&
            event_context.owner_enabled && event_context.amount==12.5f && event_context.result_type==7 &&
            event_context.event_actor_flags==(std::uint64_t{1}<<14));
        put(source,0x60,std::uint64_t{1}<<10);
        put(source,0xB40,static_cast<void*>(associated.data()));
        assert(bindings.power_cell_event_actor(event.data())==associated.data());
        associated[0xC]=std::byte{0};
        assert(!bindings.power_cell_event_actor(event.data()));
        associated[0xC]=std::byte{0xE};
        put(source,0x60,std::uint64_t{1}<<25);
        put(source,0x9E0,static_cast<void*>(associated.data()));
        assert(bindings.power_cell_event_actor(event.data())==associated.data());
        source[0xC]=std::byte{0};
        assert(!bindings.power_cell_event_actor(event.data()));
        assert(VirtualProtect(memory+0x12DB000,0x1000,previous,&ignored));
    }
    {
        std::array<std::byte,0x2C0> owner{};
        std::array<std::array<std::byte,0x448>,3> materials{};
        void* list[]={materials[0].data(),materials[1].data(),materials[2].data()};
        void** list_pointer=list;
        std::uint32_t count=3;
        std::memcpy(owner.data()+0x2B0,&list_pointer,sizeof(list_pointer));
        std::memcpy(owner.data()+0x2B8,&count,sizeof(count));
        DWORD previous=0,ignored=0;
        assert(VirtualProtect(memory+0x222F000,0x1000,PAGE_EXECUTE_READ,&previous));
        assert(FlushInstructionCache(GetCurrentProcess(),memory+0x222F910,sizeof(power_cell_material_at_code)));
        void* selected=nullptr;
        assert(select_stormbolt_power_cell_material(owner.data(),bindings.power_cell_material_at,selected));
        assert(selected==list[2]); // Native keeps the last eligible entry.
        materials[2][0x446]=std::byte{1};
        assert(select_stormbolt_power_cell_material(owner.data(),bindings.power_cell_material_at,selected));
        assert(selected==list[1]);
        materials[0][0x446]=materials[1][0x446]=std::byte{1};
        assert(select_stormbolt_power_cell_material(owner.data(),bindings.power_cell_material_at,selected));
        assert(!selected);
        count=0;std::memcpy(owner.data()+0x2B8,&count,sizeof(count));
        assert(select_stormbolt_power_cell_material(owner.data(),bindings.power_cell_material_at,selected));
        assert(!selected);
        assert(VirtualProtect(memory+0x222F000,0x1000,previous,&ignored));
    }
    {
        const std::pair<std::uintptr_t,float> constants[] = {
            {0x2E4FE54,0.5f},{0x2E4FE58,1.0f},{0x2E4FE5C,3.0f},
            {0x2F0F358,2.0f},{0x2F1FD4C,4.0f},{0x2F2504C,6.0f},{0x2F8AB90,8.0f}};
        for (const auto& item:constants) std::memcpy(memory+item.first,&item.second,sizeof(float));
        DWORD previous=0,ignored=0;
        assert(VirtualProtect(memory+0x2171000,0x1000,PAGE_EXECUTE_READ,&previous));
        assert(FlushInstructionCache(GetCurrentProcess(),memory+0x2171490,sizeof(power_cell_visual_convert_code)));
        const float inputs[][4]={{3.7f,0.6f,1.0f,0.25f},{4.0f,0.6f,1.0f,0.5f},{6.0f,0.6f,1.0f,1.0f}};
        const float expected[][3]={{0.2f,0.44f,1.0f},{0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f}};
        for (unsigned i=0;i<3;++i) {
            float output[4]={-9,-9,-9,-9};
            assert(bindings.power_cell_visual_convert(inputs[i],output)==output);
            for (unsigned channel=0;channel<3;++channel)
                assert(std::fabs(output[channel]-expected[i][channel])<0.000001f);
            assert(output[3]==inputs[i][3]);
        }
        StormboltPowerCellLoopState state{};
        const StormboltMaterialColorApi visual_api{capture_visual};
        int material=0;
        for (unsigned i=0;i<3;++i) {
            const auto band=static_cast<StormboltPowerCellVisualBand>(i);
            for (unsigned active=0;active<2;++active) {
                state.visual_phase=active;state.timer=active?5.0f:0.0f;
                const float scale=i==2?(active?700.0f:400.0f):(active?1000.0f:400.0f);
                assert(stormbolt_write_power_cell_visual_color(&material,band,state,bindings.power_cell_visual_convert,visual_api));
                assert(visual_material==&material && visual_hash==0xA5FC8881u);
                for (unsigned c=0;c<3;++c) assert(std::fabs(visual_values[c]-expected[i][c]*scale)<0.001f);
                assert(visual_values[3]==1.0f);
            }
        }
        assert(visual_writes==6);
        for (float timer : {5.0f,0.0f,-1.0f}) {
            state.timer=timer;
            assert(stormbolt_write_power_cell_threshold_scalar(&material,state,capture_scalar));
            assert(visual_material==&material && visual_hash==0xE8C621B1u);
            assert(visual_scalar==(timer>0.0f?0.9f:0.08f));
            assert(stormbolt_write_power_cell_threshold_vector(&material,state,visual_api));
            assert(visual_material==&material && visual_hash==0x58662D14u);
            const std::array<float,4> expected_vector=timer>0.0f
                ? std::array<float,4>{0.0f,40.0f,20.0f,0.0f}
                : std::array<float,4>{0.0f,40.0f,150.0f,-12.5f};
            assert(visual_values==expected_vector);
        }
        assert(!stormbolt_write_power_cell_visual_color(nullptr,StormboltPowerCellVisualBand::lower,state,bindings.power_cell_visual_convert,visual_api));
        assert(!stormbolt_write_power_cell_visual_color(&material,StormboltPowerCellVisualBand::lower,state,nullptr,visual_api));
        assert(!stormbolt_write_power_cell_visual_color(&material,StormboltPowerCellVisualBand::lower,state,bindings.power_cell_visual_convert,{}));
        assert(visual_writes==9);
        visual_order.clear();state.timer=5.0f;
        assert(stormbolt_write_power_cell_threshold_material(&material,state,
            bindings.power_cell_visual_convert,capture_scalar,visual_api));
        assert((visual_order==std::vector<std::uint32_t>{0xA5FC8881u,0xE8C621B1u,0x58662D14u}));
        state.visual_phase=0;
        assert(stormbolt_write_power_cell_non_threshold_vector(&material,state,visual_api));
        assert((visual_values==std::array<float,4>{0.0f,40.0f,150.0f,-12.5f}));
        state.visual_phase=1;
        for (const auto& sample:std::array<std::array<float,2>,4>{{{0.7f,4.0f},{0.35f,34.5f},{0.0f,65.0f},{1.4f,-57.0f}}}) {
            state.visual_timer=sample[0];
            assert(stormbolt_write_power_cell_non_threshold_vector(&material,state,visual_api));
            assert(visual_hash==0x58662D14u && visual_material==&material);
            assert(std::fabs(visual_values[0]-sample[1])<0.00001f);
            assert(visual_values[1]==40.0f && visual_values[2]==150.0f && visual_values[3]==-12.5f);
        }
        state.visual_phase=2;
        assert(stormbolt_write_power_cell_non_threshold_vector(&material,state,visual_api));
        assert((visual_values==std::array<float,4>{0.0f,0.0f,0.0f,0.0f}));
        for (auto band:{StormboltPowerCellVisualBand::lower,StormboltPowerCellVisualBand::upper}) {
            visual_order.clear();state.visual_phase=1;state.visual_timer=0.35f;
            assert(stormbolt_write_power_cell_non_threshold_material(&material,band,state,
                bindings.power_cell_visual_convert,capture_scalar,visual_api));
            assert((visual_order==std::vector<std::uint32_t>{0xA5FC8881u,0xE8C621B1u,0x58662D14u}));
            assert(visual_scalar==0.08f && visual_values[0]==34.5f);
        }
        StormboltPowerCellLoopStore<1> update_store;
        const auto update_token=update_store.initialize(reinterpret_cast<std::uintptr_t>(&material));
        StormboltLoopNativeContext context{};context.step=1.0f;
        StormboltPowerCellLoopUpdateInputs update_input{true,true,1,4,4,2};
        assert(update_store.transact(update_token,[](auto& s) noexcept {s.timer=0.5f;s.growth=true;return true;}));
        visual_order.clear();
        assert(update_stormbolt_power_cell_from_inputs(update_store,update_token,context,update_input,
            &material,false,bindings.power_cell_visual_convert,capture_scalar,visual_api,nullptr));
        StormboltPowerCellLoopState update_state{};
        assert(update_store.snapshot(update_token,update_state));
        assert(update_state.timer==-0.5f && !update_state.growth && update_state.visual_timer==-1.0f);
        assert(visual_scalar==0.9f); // Material observed timer before decrement.
        update_input.count=1;
        assert(update_store.transact(update_token,[](auto& s) noexcept {s.growth=true;return true;}));
        assert(update_stormbolt_power_cell_from_inputs(update_store,update_token,context,update_input,
            &material,false,bindings.power_cell_visual_convert,capture_scalar,visual_api,nullptr));
        assert(update_store.snapshot(update_token,update_state));
        assert(update_state.timer==-1.0f && update_state.visual_phase==1 && update_state.visual_timer==0.7f && !update_state.growth);
        update_input.resource_count=0;
        assert(update_store.transact(update_token,[](auto& s) noexcept {s.growth=true;return true;}));
        visual_order.clear();
        assert(update_stormbolt_power_cell_from_inputs(update_store,update_token,context,update_input,
            &material,false,bindings.power_cell_visual_convert,capture_scalar,visual_api,nullptr));
        assert(update_store.snapshot(update_token,update_state) && update_state.growth && visual_order.empty());
        assert(VirtualProtect(memory+0x2171000,0x1000,previous,&ignored));
    }
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_entry.mode)==base+0x2044850);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_entry.parameter)==base+0x1788D60);
    assert(bindings.last_update.event_object==bindings.middle_fire.event_object && bindings.last_update.difference==bindings.middle_prepare.difference);
    assert(bindings.last_update.origin==bindings.middle_fire.origin && bindings.last_update.fire==bindings.middle_fire.fire);
    assert(bindings.last_update.actor_flag==bindings.special_to_normal.action.actor_flag && !bindings.last_update.effect_event);
    assert(reinterpret_cast<std::uintptr_t>(bindings.last_update.complete)==base+0x1BFD260);
    assert(bindings.last_hook.action_vtable==base+0x33AA9B8 && bindings.last_hook.actor_vtable==base+0x34A6D18 &&
        bindings.last_hook.weapon_vtable==base+0x34D6C88 && !bindings.last_hook.ready);
    assert(reinterpret_cast<std::uintptr_t>(bindings.last_hook.original_enter)==base+0x23D430 &&
        reinterpret_cast<std::uintptr_t>(bindings.last_hook.original_exit)==base+0x23D430 &&
        reinterpret_cast<std::uintptr_t>(bindings.last_hook.original_update)==base+0x240720);
    assert(bindings.last_entry.mode==bindings.middle_entry.mode && bindings.last_entry.parameter==bindings.middle_entry.parameter);
    assert(reinterpret_cast<std::uintptr_t>(bindings.last_entry.request_motion)==base+0x1C014B0);
    assert(reinterpret_cast<std::uintptr_t>(bindings.local_input.query)==base+0x122FD90);
    assert(reinterpret_cast<std::uintptr_t>(bindings.update_gate.authority_slot)==base+0x4CD2AC0);
    assert(reinterpret_cast<std::uintptr_t>(bindings.update_gate.query)==base+0x5DC320);
    assert(reinterpret_cast<std::uintptr_t>(bindings.action_descriptor)==base+0x1BFF6F0);
    assert(reinterpret_cast<std::uintptr_t>(bindings.remote_attachment)==base+0x214DAB0);
    assert(reinterpret_cast<std::uintptr_t>(bindings.skill_value_query)==base+0x1922190);
    assert(reinterpret_cast<std::uintptr_t>(bindings.skill_prompt)==base+0x19A2D20);
    assert(bindings.loop_construct==base+0x12EA460 && bindings.loop_destroy==base+0x12EAA40 &&
           bindings.loop_vtable==base+0x324A830);
    assert(bindings.loop_update_site==base+0x12EE81D);
    assert(bindings.loop_inputs.skill_vtable==base+0x324B1F0 && bindings.loop_inputs.status_vtable==base+0x322C268 &&
           bindings.loop_inputs.resource_slot==base+0x5011F68 && bindings.loop_inputs.resource_vtable==base+0x31E0078);
    assert(reinterpret_cast<std::uintptr_t>(bindings.loop_default_first)==base+0x503C430);
    assert(reinterpret_cast<std::uintptr_t>(bindings.loop_default_second)==base+0x3FDE890);
    assert(reinterpret_cast<std::uintptr_t>(bindings.named_actor_event)==base+0x2300260);
    assert(reinterpret_cast<std::uintptr_t>(bindings.sound_container_at)==base+0x130C710);
    assert(bindings.power_cell.construct==base+0x11B6450 && bindings.power_cell.reset==base+0x11C34A0 &&
        bindings.power_cell.destroy==base+0x11B68A0 && bindings.power_cell.status_vtable==base+0x322C268 &&
        bindings.power_cell.resource_slot==base+0x5011F68 && bindings.power_cell.resource_vtable==base+0x31E0078);
    assert(reinterpret_cast<std::uintptr_t>(bindings.equipment_effect)==base+0xCCC620);
    assert(reinterpret_cast<std::uintptr_t>(bindings.health_adjustment)==base+0x121EA10);
    assert(bindings.weapon_update.weapon_vtable==base+0x34D6C88 && !bindings.weapon_update.ready);
    assert(reinterpret_cast<std::uintptr_t>(bindings.weapon_update.original)==base+0x21487F0);
    assert(bindings.stage_update.gate.query==bindings.update_gate.query && bindings.stage_update.descriptor==bindings.action_descriptor);
    assert(bindings.stage_update.routes.trigger.primary==bindings.input_trigger.primary);
    assert(bindings.stage_update.routes.actor_flag==bindings.special_to_normal.action.actor_flag);
    assert(bindings.stage_update.ammo_items==bindings.gatling_entry.ammo_items);
    assert(reinterpret_cast<std::uintptr_t>(bindings.input_trigger.primary)==base+0x1230070);
    assert(reinterpret_cast<std::uintptr_t>(bindings.input_trigger.alternate_gate)==base+0x1230000);
    assert(reinterpret_cast<std::uintptr_t>(bindings.material_color.write_vector)==base+0x23D82A0);
    assert(bindings.effect_color.active==bindings.cleanup_bank.active);
    assert(reinterpret_cast<std::uintptr_t>(bindings.effect_color.set_color)==base+0x22F9B70);
    assert(bindings.decay_owner.manager_slot==bindings.gatling_entry.player_manager_slot &&
        bindings.decay_owner.local_player==bindings.gatling_entry.local_player);
    assert(bindings.decay_effect.active==bindings.cleanup_bank.active && bindings.decay_effect.assign==bindings.cleanup_bank.assign);
    assert(reinterpret_cast<std::uintptr_t>(bindings.decay_effect.request)==base+0xAE2910);
    assert(bindings.cleanup_bank.active==bindings.special_to_normal.effects.bank_active &&
        bindings.cleanup_bank.assign==bindings.special_to_normal.effects.assign_bank);
    assert(reinterpret_cast<std::uintptr_t>(bindings.cleanup_bank.stop)==base+0x22F7D30);
    assert(bindings.middle_entry.needs_parameter==stormbolt_middle_needs_parameter &&
        bindings.middle_entry.read_stage==stormbolt_middle_read_stage &&
        bindings.middle_entry.write_stage==stormbolt_middle_write_stage);
    assert(bindings.middle_hook.action_vtable==base+0x33B1930 && !bindings.middle_hook.ready && !bindings.middle_hook.original_exit);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_hook.original_enter)==base+0x23D430);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_hook.original_update)==base+0x240720);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_prepare.difference)==base+0x1F62480);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_prepare.rate)==base+0x1F72CA0);
    assert(bindings.middle_prepare.animation.motion==bindings.special_to_normal.action.motion);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_fire.event_object)==base+0x1F62070);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_fire.origin)==base+0x1F62510);
    assert(reinterpret_cast<std::uintptr_t>(bindings.middle_fire.fire)==base+0x214BC20);
    assert(bindings.gatling_hook.action_vtable==base+0x33B1858 && !bindings.gatling_hook.ready && !bindings.gatling_hook.original_exit);
    assert(reinterpret_cast<std::uintptr_t>(bindings.gatling_hook.original_enter)==base+0x23D430);
    assert(reinterpret_cast<std::uintptr_t>(bindings.gatling_hook.original_update)==base+0x240720);
    assert(bindings.gatling_entry.player_manager_slot == reinterpret_cast<void**>(base+0x500ECA0));
    assert(reinterpret_cast<std::uintptr_t>(bindings.gatling_entry.local_player) == base+0x1B42010);
    assert(reinterpret_cast<std::uintptr_t>(bindings.gatling_entry.ammo_items) == base+ammo_items_rva);
    assert(bindings.gatling_entry.animation.motion == bindings.special_to_normal.action.motion);
    for (std::size_t offset=0; offset<sizeof(ammo_items_data); ++offset) {
        memory[ammo_items_rva+offset] ^= 1;
        const auto before=bindings;
        const auto result=bind_stormbolt_native_code(base,image_size,bindings);
        assert(result.status==StormboltCodeStatus::changed_data && result.rva==ammo_items_rva);
        same_bindings(before,bindings);
        memory[ammo_items_rva+offset] ^= 1;
    }
    for(std::size_t offset=0;offset<sizeof(trigger_map_data);++offset) {
        memory[trigger_map_rva+offset]^=1;
        const auto before=bindings;
        const auto result=bind_stormbolt_native_code(base,image_size,bindings);
        assert(result.status==StormboltCodeStatus::changed_data && result.rva==trigger_map_rva);
        same_bindings(before,bindings);
        memory[trigger_map_rva+offset]^=1;
    }
    DWORD data_protection=0, restored_protection=0;
    assert(VirtualProtect(memory+(ammo_items_rva & ~std::uintptr_t{0xFFF}),4096,PAGE_NOACCESS,&data_protection));
    const auto before_data=bindings;
    assert(bind_stormbolt_native_code(base,image_size,bindings).status==StormboltCodeStatus::unreadable);
    same_bindings(before_data,bindings);
    assert(VirtualProtect(memory+(ammo_items_rva & ~std::uintptr_t{0xFFF}),4096,data_protection,&restored_protection));
    assert(bindings.shell.caller_return == base + 0x214C5BE);
    assert(bindings.shell.create_entry == memory + 0x1AA74A0);
    assert(bindings.lifecycle.allocate == memory + 0x2147720);
    assert(bindings.lifecycle.place == memory + 0x21477D0);
    assert(bindings.lifecycle.destroy == memory + 0x21478F0);
    assert(bindings.coefficients.slot == memory + 0x34D7060 && bindings.coefficients.caller_return == base + 0x214CAC4);
    assert(bindings.normal_to_special.slot == memory + 0x33B17C0);
    assert(bindings.normal_to_special.action_vtable == base + 0x33B1788 &&
        bindings.normal_to_special.actor_vtable == base + 0x34A6D18 &&
        bindings.normal_to_special.weapon_vtable == base + 0x34D6C88);
    assert(bindings.normal_to_special.original == reinterpret_cast<StormboltActionUpdate>(base + 0x1892C10) &&
        bindings.normal_to_special.event_object == reinterpret_cast<StormboltActionEventObject>(base + 0x1F62070) &&
        bindings.normal_to_special.query == reinterpret_cast<StormboltActionChannelQuery>(base + 0x1BFD4E0));
    assert(bindings.detonate_update_native_inherited && bindings.primary_shoot_hook.action_vtable==base+0x33B18C0);
    const auto saved = bindings;
    const auto& special = bindings.special_to_normal;
#define AT(field, offset) assert(reinterpret_cast<std::uintptr_t>(special.field) == base + offset)
    assert(special.hook.action_vtable == base + 0x33B17F0 && special.hook.actor_vtable == base + 0x34A6D18 &&
        special.hook.weapon_vtable == base + 0x34D6C88);
    AT(hook.original_enter, 0x23D430); AT(hook.original_exit, 0x23D430); AT(hook.original_update, 0x240720);
    AT(action.motion, 0x1F72AE0); AT(action.actor_flag, 0x1F604B0); AT(action.event_object, 0x1F62070);
    AT(action.weapon_effect, 0xAE2910); AT(action.query, 0x1BFD4E0);
    AT(save.manager_slot, 0x5013950); AT(save.current_character, 0x1B8DBB0); AT(save.set_character_bit, 0x1360DA0);
    AT(effects.parameter_dti, 0x500E468); AT(effects.construct_parameter, 0x1AC6800); AT(effects.actor_selector, 0x19A7350);
    AT(effects.provider_slot, 0x51C5D48); AT(effects.provider_context, 0x502CE18);
    AT(effects.post_needed, 0xF9AD80); AT(effects.post_manager_slot, 0x4F87C90); AT(effects.post, 0xF9AA40);
    AT(effects.bank_active, 0x2360FE0); AT(effects.assign_bank, 0x2360F20); AT(effects.clear_bank70, 0xAE10F0);
#undef AT
    assert(special.action.complete() && special.save.complete());
    assert(!special.effects.create && !special.effects.complete() && !special.hook.ready);
    const auto& identity = special.provider_identity;
    assert(identity.singleton_slot == special.effects.provider_slot && identity.vtable == base+0x33F85E0 &&
        identity.dti == base+0x500E4E0 && identity.dti_vtable == base+0x33F8510 &&
        identity.dti_getter == base+0x1AC86F0 && identity.record_create == base+0x1AC9230 &&
        reinterpret_cast<std::uintptr_t>(identity.create) == base+0x1AC9750);
    // Snapshot only: fixture code pages remain non-executable; no creator is called.
    std::array<std::uintptr_t,2> provider{identity.vtable,0};
    StormboltEffectProviderSnapshot provider_output{memory+16,identity.create};
    const auto unchanged_provider = provider_output;
    const auto unchanged = [&] {
        assert(provider_output.provider == unchanged_provider.provider && provider_output.create == unchanged_provider.create);
    };
    const auto set_word = [](std::uintptr_t address, std::uintptr_t value) {
        std::memcpy(reinterpret_cast<void*>(address),&value,sizeof(value));
    };
    const auto singleton = reinterpret_cast<std::uintptr_t>(identity.singleton_slot);
    assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::absent); unchanged();
    set_word(singleton,reinterpret_cast<std::uintptr_t>(provider.data()));
    set_word(identity.dti,identity.dti_vtable);
    auto invalid_identity = identity; invalid_identity.create = nullptr;
    assert(snapshot_stormbolt_effect_provider(invalid_identity,provider_output) == StormboltProviderStatus::invalid_identity); unchanged();
    provider[0] = base+0x3515510; // Native base type is deliberately rejected.
    assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::wrong_type); unchanged();
    provider[0] = identity.vtable;
    set_word(identity.dti,identity.dti_vtable+8);
    assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::wrong_type); unchanged();
    set_word(identity.dti,identity.dti_vtable);
    for (const auto offset : {0x20u,0xF8u,0x108u}) {
        std::uintptr_t saved_word = 0;
        assert(read_stormbolt_field(identity.vtable,offset,saved_word));
        set_word(identity.vtable+offset,saved_word^8);
        assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::changed_dispatch); unchanged();
        set_word(identity.vtable+offset,saved_word);
    }
    for (unsigned i=1;i<=9;++i) {
        ProviderFaultMemory::reads=0; ProviderFaultMemory::fail_at=i;
        assert(snapshot_stormbolt_effect_provider<ProviderFaultMemory>(identity,provider_output) == StormboltProviderStatus::unreadable);
        unchanged();
    }
    for (const auto address : {singleton,reinterpret_cast<std::uintptr_t>(provider.data()),identity.dti}) {
        ProviderChangingMemory::target=address; ProviderChangingMemory::reads=0;
        assert(snapshot_stormbolt_effect_provider<ProviderChangingMemory>(identity,provider_output) == StormboltProviderStatus::changed_during_read);
        unchanged();
    }
    set_word(singleton,1);
    assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::unreadable); unchanged();
    set_word(singleton,reinterpret_cast<std::uintptr_t>(provider.data()));
    assert(snapshot_stormbolt_effect_provider(identity,provider_output) == StormboltProviderStatus::matched);
    assert(provider_output.provider == provider.data() && provider_output.create == identity.create);
    auto resolved_effect_api = special.effects;
    resolved_effect_api.provider_resolver_context = &identity;
    resolved_effect_api.resolve_provider = resolve_stormbolt_effect_provider;
    assert(resolved_effect_api.complete());
    StormboltEffectProviderSnapshot via_api{};
    assert(resolved_effect_api.resolve_provider(resolved_effect_api.provider_resolver_context,via_api));
    assert(via_api.provider == provider.data() && via_api.create == identity.create);
    assert(!special.effects.create && !special.hook.ready); // Snapshot never activates anything.
    for (const auto& span : spans) {
        const auto offset = span.rva + span.size - 1;
        memory[offset] ^= 1;
        assert(bind_stormbolt_native_code(base, image_size, bindings).status == StormboltCodeStatus::changed_code);
        same_bindings(bindings, saved);
        memory[offset] ^= 1;
    }
    for (const auto& pointer : pointers) {
        memory[pointer.slot_rva] ^= 8;
        const auto result = bind_stormbolt_native_code(base, image_size, bindings);
        assert(result.status == StormboltCodeStatus::changed_pointer && result.rva == pointer.slot_rva);
        same_bindings(bindings, saved);
        memory[pointer.slot_rva] ^= 8;
    }
    for(std::size_t n=0;n<sizeof(trigger_map_data);++n) {
        memory[trigger_map_rva+n]^=1;
        const auto result=bind_stormbolt_native_code(base,image_size,bindings);
        assert(result.status==StormboltCodeStatus::changed_data && result.rva==trigger_map_rva);
        same_bindings(bindings,saved);
        memory[trigger_map_rva+n]^=1;
    }
    DWORD old = 0;
    auto* inaccessible = memory + (spans[0].rva & ~std::uintptr_t{0xFFF});
    assert(VirtualProtect(inaccessible, 0x1000, PAGE_NOACCESS, &old));
    assert(bind_stormbolt_native_code(base, image_size, bindings).status == StormboltCodeStatus::unreadable);
    same_bindings(bindings, saved);
    DWORD temporary = 0;
    assert(VirtualProtect(inaccessible, 0x1000, old, &temporary));
    assert(bind_stormbolt_native_code(0, image_size, bindings).status == StormboltCodeStatus::invalid_image);
    assert(bind_stormbolt_native_code(base, image_size - 1, bindings).status == StormboltCodeStatus::invalid_image);
    assert(bind_stormbolt_native_code((std::numeric_limits<std::uintptr_t>::max)() - 2,
        image_size, bindings).status == StormboltCodeStatus::invalid_image);
    same_bindings(bindings, saved);
    assert(bind_stormbolt_native_code(base, image_size, bindings));
    assert(VirtualFree(memory, 0, MEM_RELEASE));
    std::puts("PASS: complete native code spans, relocated pointer fixtures, tail mutations, "
              "unreadable pages, bounds rejection and unchanged output on failure");
}
