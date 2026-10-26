#pragma once
#include "stormbolt_class48_skill_adapter.h"
#include "stormbolt_source_transition_lifecycle.h"
#include "stormbolt_source_transition_condition_hook.h"
#include "stormbolt_source_transition_native_fields.h"
#include "stormbolt_shoot_gatling_native_fields.h"
#include "stormbolt_shoot_gatling_condition.h"
#include "stormbolt_source_manager_lifecycle.h"
#include "stormbolt_condition_readiness.h"
#include "stormbolt_default_heap_readiness.h"
#include "stormbolt_evade_reload_effect.h"
#include "stormbolt_evade_effect_readiness.h"
#include "generated_stormbolt_native_code_guard.h"
#include "stormbolt_lifecycle_hooks.h"
#include "stormbolt_shell_hook.h"
#include "stormbolt_coefficient_hook.h"
#include "stormbolt_normal_to_special_hook.h"
#include "stormbolt_special_to_normal_hook.h"
#include "stormbolt_effect_provider_identity.h"
#include "stormbolt_gatling_start_entry.h"
#include "stormbolt_middle_prepare_adapter.h"
#include "stormbolt_middle_fire_adapter.h"
#include "stormbolt_middle_entry.h"
#include "stormbolt_last_entry.h"
#include "stormbolt_last_update.h"
#include "stormbolt_middle_native_fields.h"
#include "stormbolt_update_gate.h"
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_decay_effect.h"
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_effect_color.h"
#include "stormbolt_material_color.h"
#include "stormbolt_input_trigger.h"
#include "stormbolt_weapon_update_hook.h"
#include "stormbolt_self_damage_adjustment.h"
#include "stormbolt_self_damage_effect.h"
#include "stormbolt_power_cell_query.h"
#include "stormbolt_power_cell_prompt.h"
#include "stormbolt_power_cell_sound.h"
#include "stormbolt_loop_update_inputs.h"
#include "stormbolt_power_cell_visual_color.h"
#include "stormbolt_loop_component_update.h"
#include "stormbolt_status_update_dispatch.h"
#include "stormbolt_timer_update_dispatch.h"
#include "stormbolt_sync_pack_dispatch.h"
#include "stormbolt_remote_update_dispatch.h"
#include "stormbolt_power_cell_sync_codec.h"
#include "stormbolt_power_cell_growth_complete_adapter.h"
#include <array>
#include <cstring>

namespace stormslinger {
// Direct endpoints and expected derived-provider identity. Actual snapshot
// consumption and the real readiness preflight are still pending. A code check does not make
// this installable: bootstrap must validate imported locks, live manager/type
// identity, native borrowed lifetimes and motion/EFX resources separately.
struct StormboltSpecialNativeEndpoints {
    StormboltSpecialToNormalBindings hook{};
    StormboltSpecialToNormalApi action{};
    StormboltTransitionSaveApi save{};
    StormboltPlayerEffectApi effects{};
    StormboltEffectProviderIdentity provider_identity{};
};
struct StormboltPowerCellNativeIdentity {
    std::uintptr_t construct=0, reset=0, destroy=0, status_vtable=0;
    std::uintptr_t resource_slot=0, resource_vtable=0;
};
struct StormboltNativeBindings {
    std::uintptr_t evade_refill_call=0;
    std::array<unsigned char,5> evade_refill_expected{};
    StormboltEvadeReloadEffectBindings evade_effect{};
    std::uintptr_t evade_component_vtable=0;
    std::uintptr_t evade_epv_resource_vtable=0;
    std::uintptr_t evade_efx_resource_vtable=0;
    StormboltEvadeMeshIdentities evade_mesh{};
    void* source_transition_type=nullptr;
    decltype(StormboltSourceTransitionCreateApi::allocator_for) source_transition_heap=nullptr;
    StormboltDefaultHeapBindings default_heap{};
    StormboltSourceTransitionLifecycleBindings source_transition{};
    StormboltSourceTransitionConditionBindings source_transition_condition{};
    bool (*source_mine_enabled)(void*)=nullptr;
    std::int32_t (*source_command)(void*)=nullptr;
    bool (*source_platform_blocked)()=nullptr;
    StormboltShootConditionBindings shoot_condition{};
    std::uintptr_t source_manager_vtable=0;
    StormboltSourceManagerLifecycleBindings source_manager{};
    void** source_command_system_slot=nullptr;
    // PC2171490 takes input first, output second; PS4A0E050 reverses them.
    float* (*power_cell_visual_convert)(const float*,float*)=nullptr;
    StormboltMaterialAt power_cell_material_at=nullptr;
    StormboltMaterialScalar power_cell_material_scalar=nullptr;
    void* (*power_cell_event_actor)(const void*)=nullptr;
    std::uintptr_t power_cell_status_update_site=0,power_cell_status_manager_slot=0;
    std::uintptr_t power_cell_timer_update_site=0;
    std::uintptr_t power_cell_remote_update_site=0;
    std::uintptr_t power_cell_sync_construct=0,power_cell_sync_destroy=0,power_cell_sync_vtable=0;
    std::uintptr_t power_cell_sync_pack_site=0,power_cell_sync_complete_site=0;
    std::array<std::uintptr_t,3> power_cell_growth_sites{};
    std::uintptr_t class48_skill_multiplier=0;
    std::uintptr_t elemental_class48_site=0;
    std::uintptr_t elemental_ammo_class48_site=0;
    std::uintptr_t recoil_ammo40_site=0;
    bool (*power_cell_timer_native_query)(void*)=nullptr;
    std::uintptr_t loop_construct=0,loop_destroy=0,loop_vtable=0;
    std::uintptr_t damage_construct=0,damage_destroy=0,damage_update=0,damage_absorb=0,damage_vtable=0;
    std::uintptr_t loop_update_site=0;
    StormboltLoopInputBindings loop_inputs{};
    const float* loop_default_first=nullptr;
    const float* loop_default_second=nullptr;
    StormboltNamedEventRequest named_actor_event=nullptr;
    void* (*sound_container_at)(void*)=nullptr;
    StormboltSkillPrompt skill_prompt=nullptr;
    StormboltPowerCellNativeIdentity power_cell{};
    StormboltSkillValueQuery skill_value_query=nullptr;
    StormboltHealthAdjustment health_adjustment=nullptr;
    std::uintptr_t health_manager_vtable=0;
    StormboltEquipmentEffectRequest equipment_effect=nullptr;
    StormboltWeaponUpdateBindings weapon_update{};
    StormboltStageUpdateApi stage_update{};
    StormboltLocalInputApi local_input{};
    StormboltUpdateGateApi update_gate{};
    StormboltActionDescriptor action_descriptor=nullptr;
    StormboltCleanupBankApi cleanup_bank{};
    StormboltDecayEffectApi decay_effect{};
    StormboltDecayOwnerApi decay_owner{};
    StormboltEffectColorApi effect_color{};
    StormboltMaterialColorApi material_color{};
    StormboltInputTriggerApi input_trigger{};
    std::uint32_t (*remote_attachment)(void*)=nullptr;
    StormboltLifecycleEndpoints lifecycle{};
    StormboltShellHookBindings shell{};
    StormboltCoefficientBindings coefficients{};
    // Legacy field name: this body belongs to native DetonateBomb.
    StormboltNormalToSpecialBindings normal_to_special{};
    bool detonate_update_native_inherited=false;
    StormboltSpecialNativeEndpoints special_to_normal{};
    StormboltGatlingEntryApi gatling_entry{};
    StormboltSpecialToNormalBindings gatling_hook{};
    StormboltSpecialToNormalBindings last_hook{};
    StormboltLastEntryApi last_entry{};
    StormboltLastUpdateApi last_update{};
    StormboltMiddleEntryApi middle_entry{};
    StormboltMiddlePrepareApi middle_prepare{};
    StormboltMiddleFireApi middle_fire{};
    StormboltSpecialToNormalBindings middle_hook{},primary_shoot_hook{};
};
enum class StormboltCodeStatus { matched, invalid_image, unreadable, changed_code, changed_pointer, changed_data };
struct StormboltCodeCheck {
    StormboltCodeStatus status = StormboltCodeStatus::invalid_image;
    std::uintptr_t rva = 0;
    explicit operator bool() const noexcept { return status == StormboltCodeStatus::matched; }
};

// Read-only live bytes/pointers check. This does not authenticate the executable
// on disk, launch the game, activate any hooks or assert resource readiness.
// Bootstrap must authenticate the version separately before using the output.
// Run at a quiescent point before any of these hook sites are patched.
inline StormboltCodeCheck bind_stormbolt_native_code(std::uintptr_t image, std::size_t image_bytes,
                                                    StormboltNativeBindings& output) noexcept {
    using namespace native_shooting_guard;
    static_assert(sizeof(spans) / sizeof(spans[0]) == 188 && sizeof(pointers) / sizeof(pointers[0]) == 72 && sizeof(trigger_map_data)==548,
        "Regenerate complete lifecycle, shell, coefficient and action guards before binding native endpoints");
    if (!image || image_bytes != image_size ||
        image_bytes > (std::numeric_limits<std::uintptr_t>::max)() - image)
        return {};
    std::array<unsigned char, 256> copied{};
    for (const auto& span : spans) {
        if (span.rva >= image_bytes || span.size > image_bytes - span.rva) return {};
        for (std::size_t offset = 0; offset < span.size;) {
            const auto count = (std::min)(copied.size(), span.size - offset);
            SIZE_T read = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(image + span.rva + offset),
                                   copied.data(), count, &read) || read != count)
                return {StormboltCodeStatus::unreadable, span.rva + offset};
            if (std::memcmp(copied.data(), span.bytes + offset, count))
                return {StormboltCodeStatus::changed_code, span.rva + offset};
            offset += count;
        }
    }
    for (const auto& slot : pointers) {
        std::uintptr_t value = 0;
        if (slot.slot_rva > image_bytes - sizeof(value) || slot.value_rva >= image_bytes) return {};
        if (!read_stormbolt_field(image, slot.slot_rva, value))
            return {StormboltCodeStatus::unreadable, slot.slot_rva};
        if (value != image + slot.value_rva) return {StormboltCodeStatus::changed_pointer, slot.slot_rva};
    }
    std::array<unsigned char, sizeof(trigger_map_data)> trigger_copy{};
    if(trigger_map_rva>=image_bytes || trigger_copy.size()>image_bytes-trigger_map_rva) return {};
    SIZE_T trigger_read=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(image+trigger_map_rva),
        trigger_copy.data(),trigger_copy.size(),&trigger_read) || trigger_read!=trigger_copy.size())
        return {StormboltCodeStatus::unreadable,trigger_map_rva};
    if(std::memcmp(trigger_copy.data(),trigger_map_data,trigger_copy.size()))
        return {StormboltCodeStatus::changed_data,trigger_map_rva};
    std::array<unsigned char, sizeof(ammo_items_data)> ammo_copy{};
    if (ammo_items_rva >= image_bytes || ammo_copy.size() > image_bytes-ammo_items_rva) return {};
    SIZE_T ammo_read = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(image+ammo_items_rva),
        ammo_copy.data(), ammo_copy.size(), &ammo_read) || ammo_read != ammo_copy.size())
        return {StormboltCodeStatus::unreadable, ammo_items_rva};
    if (std::memcmp(ammo_copy.data(), ammo_items_data, ammo_copy.size()))
        return {StormboltCodeStatus::changed_data, ammo_items_rva};
    StormboltNativeBindings candidate{};
    candidate.power_cell_visual_convert=reinterpret_cast<decltype(candidate.power_cell_visual_convert)>(image+0x2171490);
    candidate.power_cell_material_at=reinterpret_cast<StormboltMaterialAt>(image+0x222F910);
    candidate.power_cell_material_scalar=reinterpret_cast<StormboltMaterialScalar>(image+0x23D8180);
    candidate.power_cell_event_actor=reinterpret_cast<decltype(candidate.power_cell_event_actor)>(image+0x12DBE50);
    candidate.power_cell_status_update_site=image+0x11C6CE3;
    candidate.power_cell_status_manager_slot=image+0x500ECA0;
    candidate.power_cell_timer_update_site=image+0x11C7F18;
    candidate.power_cell_remote_update_site=image+0x11B034A;
    candidate.power_cell_sync_construct=image+0x11927A0;
    candidate.power_cell_sync_destroy=image+0x1193610;
    candidate.power_cell_sync_vtable=image+0x322BBB0;
    candidate.power_cell_sync_pack_site=image+0x1199E99;
    candidate.power_cell_sync_complete_site=image+0x1199F4B;
    candidate.class48_skill_multiplier=image+0x11BC1D0;
    candidate.elemental_class48_site=image+0x1F6797B;
    candidate.elemental_ammo_class48_site=image+0x11BA5DB;
    candidate.recoil_ammo40_site=image+0x214D03D;
    candidate.source_transition_type=reinterpret_cast<void*>(image+0x4F7E7D0);
    candidate.source_transition_heap=reinterpret_cast<decltype(candidate.source_transition_heap)>(image+0x216F3F0);
    candidate.evade_refill_call=image+0x187A2AE;
    candidate.evade_refill_expected={{0xE8,0xFD,0xE5,0x8C,0x00}};
    candidate.evade_effect={reinterpret_cast<void(*)(void*)>(image+0x21488B0),
        reinterpret_cast<decltype(candidate.evade_effect.weapon_effect)>(image+0xAE2910),nullptr,nullptr,0x1D00};
    candidate.evade_component_vtable=image+0x3118D20;
    candidate.evade_epv_resource_vtable=image+0x3518FD8;
    candidate.evade_efx_resource_vtable=image+0x3525848;
    candidate.evade_mesh={image+0x3551948,image+0x357CF90,image+0x351FC98,
        image+0x3507A18,image+0x357D260,image+0x351A078,image+0x51C73D0,image+0x3512160,
        image+0x357D008,image+0x35899C8};
    candidate.default_heap={image+0x50EC920,image+0x34DF340,image+0x21F1CC0,image+0x21F1DA0,
        image+0x32575B0,image+0x21F2BB0,image+0x21F3150};
    candidate.source_transition={image+0x31C4A40,
        reinterpret_cast<decltype(candidate.source_transition.original_create)>(image+0xF03CA0),
        reinterpret_cast<decltype(candidate.source_transition.original_place)>(image+0xF03D20),
        reinterpret_cast<decltype(candidate.source_transition.original_destroy)>(image+0xF0AE50)};
    candidate.source_transition_condition={image+0x31C92B0,image+0x34A6D18,image+0x34D6C88,
        reinterpret_cast<decltype(candidate.source_transition_condition.original)>(image+0xF13060),nullptr};
    candidate.source_mine_enabled=reinterpret_cast<decltype(candidate.source_mine_enabled)>(image+0x2147A80);
    candidate.source_command=reinterpret_cast<decltype(candidate.source_command)>(image+0x17BE130);
    candidate.source_platform_blocked=reinterpret_cast<decltype(candidate.source_platform_blocked)>(image+0x240720);
    candidate.shoot_condition.type=reinterpret_cast<void*>(image+0x4F9F240);
    candidate.shoot_condition.create=reinterpret_cast<decltype(candidate.shoot_condition.create)>(image+0x11CB370);
    candidate.shoot_condition.initialize=reinterpret_cast<decltype(candidate.shoot_condition.initialize)>(image+0x11CB480);
    candidate.shoot_condition.destroy=reinterpret_cast<decltype(candidate.shoot_condition.destroy)>(image+0xEF3CA0);
    candidate.shoot_condition.vtable=reinterpret_cast<void**>(image+0x31C0AA0);
    candidate.shoot_condition.actor_vtable=image+0x34A6D18;
    candidate.shoot_condition.weapon_vtable=image+0x34D6C88;
    // Local human controller is the derived 11927A0 type; base helpers retain their detours.
    candidate.source_manager_vtable=image+0x322BBB0;
    candidate.source_command_system_slot=reinterpret_cast<void**>(image+0x51C4640);
    candidate.source_manager={reinterpret_cast<decltype(candidate.source_manager.initialize)>(image+0x118DCA0),
        reinterpret_cast<decltype(candidate.source_manager.rebuild)>(image+0x118DDC0),
        reinterpret_cast<decltype(candidate.source_manager.tick)>(image+0x118DBB0),
        reinterpret_cast<decltype(candidate.source_manager.destroy)>(image+0x118E810)};
    candidate.power_cell_growth_sites={{image+0x121B37F,image+0x121B39E,image+0x121B3A7}};
    candidate.power_cell_timer_native_query=reinterpret_cast<decltype(candidate.power_cell_timer_native_query)>(image+0x1F628A0);
    candidate.loop_construct=image+0x12EA460;
    candidate.damage_construct=image+0x1216EF0;
    candidate.damage_destroy=image+0x1217340;
    candidate.damage_update=image+0x121B8D0;
    candidate.damage_absorb=image+0x118EFE0;
    candidate.damage_vtable=image+0x3240FB0;
    candidate.loop_destroy=image+0x12EAA40;
    candidate.loop_vtable=image+0x324A830;
    candidate.loop_update_site=image+0x12EE81D;
    candidate.loop_inputs={image+0x324B1F0,image+0x322C268,image+0x5011F68,image+0x31E0078,image+0x34D6C88};
    candidate.loop_default_first=reinterpret_cast<const float*>(image+0x503C430);
    candidate.loop_default_second=reinterpret_cast<const float*>(image+0x3FDE890);
    candidate.named_actor_event=reinterpret_cast<StormboltNamedEventRequest>(image+0x2300260);
    candidate.sound_container_at=reinterpret_cast<void* (*)(void*)>(image+0x130C710);
    candidate.power_cell={image+0x11B6450,image+0x11C34A0,image+0x11B68A0,image+0x322C268,
        image+0x5011F68,image+0x31E0078};
    candidate.skill_prompt=reinterpret_cast<StormboltSkillPrompt>(image+0x19A2D20);
    candidate.skill_value_query=reinterpret_cast<StormboltSkillValueQuery>(image+0x1922190);
    candidate.equipment_effect=reinterpret_cast<StormboltEquipmentEffectRequest>(image+0xCCC620);
    candidate.health_adjustment=reinterpret_cast<StormboltHealthAdjustment>(image+0x121EA10);
    candidate.health_manager_vtable=image+0x3240EA0;
    candidate.lifecycle = {reinterpret_cast<void*>(image + 0x2147720),
        reinterpret_cast<void*>(image + 0x21477D0), reinterpret_cast<void*>(image + 0x21478F0),
        reinterpret_cast<void*>(image + 0x2147840),0x2860};
    candidate.shell = {reinterpret_cast<void*>(image + 0x1AA74A0), image + 0x214C5BE,
        image + 0x34D6C88, image + 0x33EBDF0,
        reinterpret_cast<StormboltShellRecordAt>(image + 0xF7DC00),nullptr,image+0x3251358};
    candidate.coefficients = {reinterpret_cast<void*>(image + 0x34D7060),
        reinterpret_cast<StormboltCoefficientCallback>(image + 0x23D430), image + 0x214CAC4,
        image + 0x34D6C88, image + 0x3119240, image + 0x3251358};
    candidate.normal_to_special = {reinterpret_cast<void*>(image + 0x33B17C0),
        reinterpret_cast<StormboltActionUpdate>(image + 0x1892C10),
        image + 0x33B1788, image + 0x34A6D18, image + 0x34D6C88,
        reinterpret_cast<StormboltActionEventObject>(image + 0x1F62070),
        reinterpret_cast<StormboltActionChannelQuery>(image + 0x1BFD4E0)};
    candidate.detonate_update_native_inherited=true;
    // PS VT6778A58 is NormalToSpecial; retain legacy core names temporarily.
    auto& special = candidate.special_to_normal;
    special.hook = {image + 0x33B17F0, image + 0x34A6D18, image + 0x34D6C88,
        reinterpret_cast<StormboltActionBoundary>(image + 0x23D430),
        reinterpret_cast<StormboltActionBoundary>(image + 0x23D430),
        reinterpret_cast<StormboltActionUpdate>(image + 0x240720), nullptr};
    special.action = {
        reinterpret_cast<decltype(special.action.motion)>(image + 0x1F72AE0),
        reinterpret_cast<decltype(special.action.actor_flag)>(image + 0x1F604B0),
        reinterpret_cast<decltype(special.action.event_object)>(image + 0x1F62070),
        reinterpret_cast<decltype(special.action.weapon_effect)>(image + 0xAE2910),
        reinterpret_cast<decltype(special.action.query)>(image + 0x1BFD4E0)};
    special.save = {reinterpret_cast<void**>(image + 0x5013950),
        reinterpret_cast<decltype(special.save.current_character)>(image + 0x1B8DBB0),
        reinterpret_cast<decltype(special.save.set_character_bit)>(image + 0x1360DA0)};
    special.effects = {reinterpret_cast<void*>(image + 0x500E468),
        reinterpret_cast<decltype(special.effects.construct_parameter)>(image + 0x1AC6800),
        reinterpret_cast<decltype(special.effects.actor_selector)>(image + 0x19A7350),
        reinterpret_cast<void**>(image + 0x51C5D48), reinterpret_cast<void*>(image + 0x502CE18),
        nullptr, // Dynamic provider virtual108: must not guess an implementation.
        reinterpret_cast<decltype(special.effects.post_needed)>(image + 0xF9AD80),
        reinterpret_cast<void**>(image + 0x4F87C90),
        reinterpret_cast<decltype(special.effects.post)>(image + 0xF9AA40),
        reinterpret_cast<decltype(special.effects.bank_active)>(image + 0x2360FE0),
        reinterpret_cast<decltype(special.effects.assign_bank)>(image + 0x2360F20),
        reinterpret_cast<decltype(special.effects.clear_bank70)>(image + 0xAE10F0)};
    special.provider_identity = {special.effects.provider_slot, image + 0x33F85E0,
        image + 0x500E4E0, image + 0x33F8510, image + 0x1AC86F0, image + 0x1AC9230,
        reinterpret_cast<StormboltEffectCreate>(image + 0x1AC9750)};
    candidate.gatling_entry.animation = special.action;
    candidate.gatling_entry.player_manager_slot = reinterpret_cast<void**>(image+0x500ECA0);
    candidate.gatling_entry.local_player = reinterpret_cast<decltype(candidate.gatling_entry.local_player)>(image+0x1B42010);
    candidate.gatling_entry.ammo_items = reinterpret_cast<const std::uint32_t*>(image+ammo_items_rva);
    // PS VT6778B10 is SpecialToNormal (exit), not ShootLGatling.
    candidate.gatling_hook = {image+0x33B1858, image+0x34A6D18, image+0x34D6C88,
        reinterpret_cast<StormboltActionBoundary>(image+0x23D430), nullptr,
        reinterpret_cast<StormboltActionUpdate>(image+0x240720), nullptr};
    // Native wrapper ABI: actor in RCX, parameter in XMM1, mode in R8D; bool in AL.
    candidate.middle_entry.mode = reinterpret_cast<decltype(candidate.middle_entry.mode)>(image+0x2044850);
    candidate.middle_entry.parameter = reinterpret_cast<decltype(candidate.middle_entry.parameter)>(image+0x1788D60);
    candidate.middle_entry.needs_parameter = stormbolt_middle_needs_parameter;
    candidate.middle_entry.read_stage = stormbolt_middle_read_stage;
    candidate.middle_entry.write_stage = stormbolt_middle_write_stage;
    candidate.middle_prepare = {special.action,
        reinterpret_cast<decltype(candidate.middle_prepare.difference)>(image+0x1F62480),
        reinterpret_cast<decltype(candidate.middle_prepare.rate)>(image+0x1F72CA0)};
    candidate.middle_fire = {
        reinterpret_cast<decltype(candidate.middle_fire.event_object)>(image+0x1F62070),
        reinterpret_cast<decltype(candidate.middle_fire.origin)>(image+0x1F62510),
        reinterpret_cast<decltype(candidate.middle_fire.fire)>(image+0x214BC20)};
    candidate.middle_hook = {image+0x33B1930,image+0x34A6D18,image+0x34D6C88,
        reinterpret_cast<StormboltActionBoundary>(image+0x23D430),nullptr,
        reinterpret_cast<StormboltActionUpdate>(image+0x240720),nullptr};
    // PS ShootLGatling and Middle share entry3A11B10/update3A11BC0.
    candidate.primary_shoot_hook=candidate.middle_hook;
    candidate.primary_shoot_hook.action_vtable=image+0x33B18C0;
    candidate.last_hook = {image+0x33AA9B8,image+0x34A6D18,image+0x34D6C88,
        reinterpret_cast<StormboltActionBoundary>(image+0x23D430),
        reinterpret_cast<StormboltActionBoundary>(image+0x23D430),
        reinterpret_cast<StormboltActionUpdate>(image+0x240720),nullptr};
    candidate.last_entry = {candidate.middle_entry.mode,candidate.middle_entry.parameter,
        reinterpret_cast<decltype(candidate.last_entry.request_motion)>(image+0x1C014B0)};
    candidate.last_update = {candidate.middle_fire.event_object,candidate.middle_prepare.difference,
        candidate.middle_fire.origin,candidate.middle_fire.fire,special.action.actor_flag,
        nullptr, // Session supplies its owned effect adapter; no process-global context.
        reinterpret_cast<decltype(candidate.last_update.complete)>(image+0x1BFD260)};
    candidate.local_input.query = reinterpret_cast<decltype(candidate.local_input.query)>(image+0x122FD90);
    candidate.update_gate = {reinterpret_cast<void**>(image+0x4CD2AC0),
        reinterpret_cast<decltype(candidate.update_gate.query)>(image+0x5DC320)};
    candidate.action_descriptor = reinterpret_cast<StormboltActionDescriptor>(image+0x1BFF6F0);
    candidate.cleanup_bank = {special.effects.bank_active,
        reinterpret_cast<decltype(candidate.cleanup_bank.stop)>(image+0x22F7D30),special.effects.assign_bank};
    candidate.decay_effect = {special.effects.bank_active,
        reinterpret_cast<decltype(candidate.decay_effect.request)>(image+0xAE2910),special.effects.assign_bank};
    candidate.decay_owner = {candidate.gatling_entry.player_manager_slot,candidate.gatling_entry.local_player};
    candidate.remote_attachment=reinterpret_cast<decltype(candidate.remote_attachment)>(image+0x214DAB0);
    candidate.input_trigger={reinterpret_cast<decltype(candidate.input_trigger.primary)>(image+0x1230070),
        reinterpret_cast<decltype(candidate.input_trigger.alternate_gate)>(image+0x1230000)};
    candidate.material_color = {reinterpret_cast<decltype(candidate.material_color.write_vector)>(image+0x23D82A0)};
    candidate.effect_color = {special.effects.bank_active,
        reinterpret_cast<decltype(candidate.effect_color.set_color)>(image+0x22F9B70)};
    candidate.weapon_update={image+0x34D6C88,reinterpret_cast<StormboltWeaponUpdate>(image+0x21487F0),nullptr};
    candidate.stage_update={candidate.update_gate,candidate.action_descriptor,
        {candidate.local_input,candidate.input_trigger,candidate.remote_attachment,candidate.decay_effect,
         candidate.cleanup_bank,candidate.decay_owner,special.action.actor_flag,
         candidate.effect_color,candidate.material_color},candidate.gatling_entry.ammo_items};
    output = candidate; // Failure leaves previously supplied bindings unchanged.
    return {StormboltCodeStatus::matched, 0};
}
inline StormboltClass48SkillBindings stormbolt_class48_skill_bindings(const StormboltNativeBindings& native) noexcept {
    return {native.power_cell.status_vtable,native.loop_inputs.resource_slot,native.loop_inputs.resource_vtable,native.skill_value_query};
}
inline StormboltTimerUpdateBindings stormbolt_timer_update_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.loop_inputs,native.skill_value_query,native.skill_prompt,
        native.power_cell_timer_native_query};
}
inline StormboltStatusUpdateBindings stormbolt_status_update_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.loop_inputs,native.skill_value_query,native.special_to_normal.action.actor_flag,
        native.power_cell_status_manager_slot};
}
inline StormboltLoopComponentUpdateBindings stormbolt_loop_component_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {{native.loop_vtable,native.sound_container_at,native.loop_default_first,
        native.loop_default_second,native.loop_inputs,native.skill_value_query,native.named_actor_event},
        native.power_cell_material_at,native.power_cell_visual_convert,
        native.power_cell_material_scalar,native.material_color};
}
inline StormboltSyncPackBindings stormbolt_sync_pack_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.power_cell_sync_vtable,native.loop_inputs,native.skill_value_query,
        &encode_stormbolt_power_cell_sync};
}
inline StormboltRemoteUpdateBindings stormbolt_remote_update_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.loop_inputs,stormbolt_loop_component_bindings(native).audio,
        native.gatling_entry.player_manager_slot,native.gatling_entry.local_player,
        native.equipment_effect,&decode_stormbolt_power_cell_sync};
}
// Bootstrap supplies the authenticated LIVE heap virtual38, whose identity is
// dynamic and is not established by this executable-code guard.
inline StormboltShootGatlingNativeApi stormbolt_shoot_gatling_field_api(
    const StormboltNativeBindings& native) noexcept {
    return {native.local_input.query,native.input_trigger.primary,native.coefficients.parameter_vtable};
}
inline StormboltShootConditionBindings stormbolt_shoot_gatling_condition_api(
    const StormboltNativeBindings& native,bool (*ready)(void*,void*,void*)=nullptr,
    void* context=nullptr) noexcept {
    auto api=native.shoot_condition;
    api.fields=stormbolt_shoot_gatling_field_api(native);
    api.ready=ready;api.ready_context=context;return api;
}
inline StormboltConditionReadinessBindings stormbolt_condition_readiness_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.normal_to_special.actor_vtable,native.shell.weapon_vtable,native.coefficients.parameter_vtable,
        native.source_manager_vtable,native.update_gate.authority_slot,native.source_command_system_slot,
        native.gatling_entry.player_manager_slot,false};
}
inline StormboltEvadeEffectReadinessBindings stormbolt_evade_effect_readiness_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {native.shell.weapon_vtable,native.normal_to_special.actor_vtable,native.evade_component_vtable,
        native.special_to_normal.provider_identity,false,nullptr,nullptr,
        native.evade_epv_resource_vtable,native.evade_efx_resource_vtable,native.evade_mesh};
}
inline StormboltSourceManagerBindings stormbolt_source_manager_api(
    const StormboltNativeBindings& native,bool (*ready)(void*,void*,void*)=nullptr,
    void* context=nullptr) noexcept {
    return {native.source_manager_vtable,stormbolt_shoot_gatling_condition_api(native,ready,context)};
}
inline StormboltSourceTransitionCreateApi stormbolt_source_transition_create_api(
    const StormboltNativeBindings& native,
    decltype(StormboltSourceTransitionCreateApi::allocate) allocate) noexcept {
    return {native.source_transition_type,native.source_transition_heap,allocate,
        native.source_transition.original_place};
}
inline StormboltSourceTransitionPredicateApi stormbolt_source_transition_field_api(
    const StormboltNativeBindings& native,StormboltSourceTransitionPredicateApi api={}) noexcept {
    api=stormbolt_source_native_field_api(api);
    api.mine_enabled=native.source_mine_enabled;
    api.command=native.source_command;
    api.native_platform_blocked=native.source_platform_blocked;
    api.native_authority_slot=native.update_gate.authority_slot;
    api.native_authority_query=native.update_gate.query;
    // Complete pointers do not establish live singleton/context readiness.
    return api;
}
inline StormboltPowerCellCompleteGrowthBindings stormbolt_complete_growth_bindings(
    const StormboltNativeBindings& native) noexcept {
    return {{native.damage_vtable,stormbolt_loop_component_bindings(native).audio,
        native.skill_prompt,native.gatling_entry.player_manager_slot,native.gatling_entry.local_player},
        native.power_cell_sync_vtable,native.equipment_effect};
}
} // namespace stormslinger
