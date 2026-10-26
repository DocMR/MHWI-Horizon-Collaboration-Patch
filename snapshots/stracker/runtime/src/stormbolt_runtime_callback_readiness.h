#pragma once
#include "stormbolt_native_bindings.h"

namespace stormslinger {
enum class StormboltRuntimeAction { special,gatling,middle,last,primary_shoot };
// Owned callback inputs. The host's synchronous native borrow remains required;
// object/page checks cannot establish it. The dependency callback must cover all
// additional stage resources used by these actions as well as the shared EPV chain.
class StormboltRuntimeCallbackReadiness {
    StormboltNativeBindings native_;
    StormboltEvadeEffectReadiness resources_;
    bool borrowed_;
    bool owner_for(std::uintptr_t weapon,std::uintptr_t& actor) const {
        std::uintptr_t table=0,current=0;std::uint8_t alive=0;std::uint32_t category=0,id=0;
        return borrowed_ && native_.shell.weapon_vtable && native_.normal_to_special.actor_vtable &&
            read_stormbolt_field(weapon,0,table) && table==native_.shell.weapon_vtable &&
            read_stormbolt_field(weapon,0xC,alive) && (alive&0xE) &&
            read_stormbolt_field(weapon,0x9F8,category) && category==13 &&
            read_stormbolt_field(weapon,0x9FC,id) && (id==238 || id==239) &&
            read_stormbolt_field(weapon,0x9B0,actor) && actor &&
            read_stormbolt_field(actor,0,table) && table==native_.normal_to_special.actor_vtable &&
            read_stormbolt_field(actor,0xC,alive) && (alive&0xE) &&
            read_stormbolt_field(actor,0x76B0,current) && current==weapon;
    }
    bool dependencies(std::uintptr_t weapon,std::uintptr_t actor) const {
        std::uintptr_t parameter=0,table=0,manager=0;
        if(!native_.coefficients.parameter_vtable || !read_stormbolt_field(actor,0x7810,parameter) || !parameter ||
            !read_stormbolt_field(parameter,0,table) || table!=native_.coefficients.parameter_vtable ||
            !stormbolt_native_span(parameter,0x1600,9*40) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(native_.gatling_entry.player_manager_slot),0,manager) || !manager ||
            !native_.special_to_normal.action.event_object) return false;
        auto* event=native_.special_to_normal.action.event_object(reinterpret_cast<void*>(actor));
        if(!stormbolt_native_span(reinterpret_cast<std::uintptr_t>(event),0x178,0x20,true)) return false;
        std::uintptr_t after=0;
        return owner_for(weapon,after) && after==actor && resources_.ready(reinterpret_cast<void*>(weapon));
    }
public:
    StormboltRuntimeCallbackReadiness(const StormboltNativeBindings& native,
        const StormboltEvadeEffectReadinessBindings& resources,bool borrowed)
        :native_(native),resources_(resources),borrowed_(borrowed && resources.synchronous_borrow_verified) {}
    bool weapon(void* value) const {
        const auto address=reinterpret_cast<std::uintptr_t>(value);std::uintptr_t actor=0;
        return owner_for(address,actor) && dependencies(address,actor);
    }
    bool action(void* value,StormboltSpecialActionPhase phase,StormboltRuntimeAction kind) const {
        std::uintptr_t expected=0;
        switch(kind) {
        case StormboltRuntimeAction::special:expected=native_.special_to_normal.hook.action_vtable;break;
        case StormboltRuntimeAction::gatling:expected=native_.gatling_hook.action_vtable;break;
        case StormboltRuntimeAction::middle:expected=native_.middle_hook.action_vtable;break;
        case StormboltRuntimeAction::last:expected=native_.last_hook.action_vtable;break;
        case StormboltRuntimeAction::primary_shoot:expected=native_.primary_shoot_hook.action_vtable;break;
        }
        const auto address=reinterpret_cast<std::uintptr_t>(value);
        std::uintptr_t table=0,actor=0,weapon_address=0,owner=0;
        if(!expected || !read_stormbolt_field(address,0,table) || table!=expected ||
            !read_stormbolt_field(address,0x28,actor) || !actor ||
            !read_stormbolt_field(actor,0x76B0,weapon_address) ||
            !owner_for(weapon_address,owner) || owner!=actor) return false;
        // Exit must remain available when effects/managers have become unavailable.
        if(phase==StormboltSpecialActionPhase::exit) return true;
        if(phase!=StormboltSpecialActionPhase::enter && phase!=StormboltSpecialActionPhase::update) return false;
        return dependencies(weapon_address,actor);
    }
};
}
