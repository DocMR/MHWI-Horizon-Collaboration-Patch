#pragma once
#include "stormbolt_self_damage_adjustment.h"
#include "stormbolt_self_damage_amount.h"
#include "stormbolt_self_damage_branch.h"
#include "stormbolt_power_cell_self_damage_state.h"
#include "stormbolt_power_cell_growth_effects.h"
#include "stormbolt_self_damage_effect.h"

namespace stormslinger {
struct StormboltSelfDamageShotBindings {
    std::uintptr_t parameter_vtable{},damage_vtable{},health_vtable{};
    StormboltLoopAudioUpdateBindings loop{};
    StormboltHealthAdjustment adjust{};
    StormboltSkillPrompt prompt{};
    StormboltEquipmentEffectRequest equipment{};
};
// PS4998470: snapshot is already clamped by the factory. Health/state effects
// run before energy subtraction and creation. Borrowing belongs to that caller.
template<std::size_t S,std::size_t L>
bool apply_stormbolt_shot_self_damage(void* weapon,std::uint32_t snapshot,
    StormboltPowerCellStore<S>& statuses,StormboltPowerCellLoopStore<L>& loops,
    const StormboltSelfDamageShotBindings& api) {
    if(snapshot>8 || !api.adjust)return false;
    const auto w=reinterpret_cast<std::uintptr_t>(weapon);
    std::uintptr_t owner{},parameter{},table{},status{},associated{};
    std::uint8_t live{};
    if(!read_stormbolt_field(w,0x9B0,owner) ||
        !read_stormbolt_field(owner,0xC,live) || !(live&0xE) ||
        !read_stormbolt_field(owner,0x7810,parameter) ||
        !read_stormbolt_field(parameter,0,table) || table!=api.parameter_vtable ||
        !read_stormbolt_field(owner,0x7D20,status) ||
        !read_stormbolt_field(status,0,table) || table!=api.loop.inputs.status_vtable ||
        !read_stormbolt_field(status,8,associated))return false;
    StormboltPowerCellLoopUpdateInputs skill{};
    if(associated && !read_stormbolt_loop_skill_gate(reinterpret_cast<void*>(associated),
        api.loop.inputs.skill_vtable,api.loop.query,skill))return false;
    const bool powered=associated && skill.owner_flag && skill.resource_index_present && skill.resource_count>0;
    StormboltPowerCellToken token{};StormboltPowerCellState state{};
    StormboltPowerCellParameters power{};
    if(powered) {
        std::uintptr_t resource{};
        if(!statuses.token_for_live_address(status,token) || !statuses.snapshot(token,state) ||
            !read_stormbolt_field(api.loop.inputs.resource_slot,0,resource) ||
            !read_stormbolt_field(resource,0,table) || table!=api.loop.inputs.resource_vtable ||
            !read_stormbolt_field(resource,0xA8C,power.threshold) ||
            !read_stormbolt_field(resource,0xA94,power.duration))return false;
    }
    const auto branch=stormbolt_self_damage_branch(snapshot,associated!=0,skill.owner_flag,
        skill.resource_index_present?1:0,static_cast<std::int16_t>(skill.resource_count),
        state.count,power.threshold);
    float amount{};
    if(branch.conditional && snapshot==8) {
        std::uintptr_t health{};
        if(!read_stormbolt_field(owner,0x7630,health) ||
            !read_stormbolt_field(health,0,table) || table!=api.health_vtable ||
            !read_stormbolt_field(health,0x64,amount))return false;
        amount*=0.5f;
    } else if(!read_stormbolt_field(parameter,
        (branch.conditional?0x1618:0x1614)+40*snapshot,amount))return false;
    if(branch.raise_state_and_notify) {
        const auto promotion=promote_stormbolt_power_cell_for_self_damage(
            statuses,token,true,snapshot,power);
        if(promotion==StormboltPowerCellPromotion::unavailable)return false;
        if(promotion==StormboltPowerCellPromotion::promoted) {
            if(!apply_stormbolt_power_cell_growth_effects(
                StormboltPowerCellGrowthResult::reached_threshold,
                reinterpret_cast<void*>(associated),loops,api.loop,api.prompt))return false;
            std::uintptr_t primary{};
            if(!read_stormbolt_field(associated,0x13D58,primary) || !primary ||
                !stormbolt_self_damage_effect(reinterpret_cast<void*>(primary),api.equipment).invoked)return false;
        }
    }
    if(!(amount>0))return true;
    std::uintptr_t damage{};
    if(!read_stormbolt_field(w,0x9B0,owner) ||
        !read_stormbolt_field(owner,0xC,live) || !(live&0xE) ||
        !read_stormbolt_field(owner,0x7658,damage) ||
        !read_stormbolt_field(damage,0,table) || table!=api.damage_vtable)return false;
    return stormbolt_adjust_self_damage(reinterpret_cast<void*>(damage),amount,api.adjust)==
        StormboltSelfDamageDispatch::invoked;
}
}
