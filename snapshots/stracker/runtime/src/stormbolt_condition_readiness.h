#pragma once
#include "stormbolt_native_memory.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltConditionReadinessBindings {
    std::uintptr_t actor_vtable=0,weapon_vtable=0,parameter_vtable=0,manager_vtable=0;
    void** authority_slot=nullptr;
    void** command_system_slot=nullptr;
    void** player_manager_slot=nullptr;
    // Bootstrap must establish the synchronous native FSM borrow/exclusion.
    // Page checks below cannot turn an arbitrary pointer into an owned object.
    bool synchronous_borrow_verified=false;
};
template<std::size_t Capacity>
class StormboltConditionReadiness {
    StormboltInstanceStore<Capacity>& store_;
    StormboltConditionReadinessBindings api_;
    bool (*dispatch_ready_)(void*)=nullptr;
    void* dispatch_context_=nullptr;
    struct Context {std::uintptr_t actor=0,weapon=0,root=0,input=0,manager=0,physical=0,parameter=0;};
    bool common(void* transition,void* weapon,Context& c) const {
        if(!api_.synchronous_borrow_verified || !api_.actor_vtable || !api_.weapon_vtable || !api_.manager_vtable ||
            (dispatch_ready_ && !dispatch_ready_(dispatch_context_))) return false;
        c.weapon=reinterpret_cast<std::uintptr_t>(weapon);
        const auto t=reinterpret_cast<std::uintptr_t>(transition);
        std::uintptr_t table=0,owner=0,current_weapon=0,input_actor=0,root=0;
        std::uint8_t alive=0;std::uint32_t category=0,id=0;
        if(!read_stormbolt_field(t,0x10,c.actor) || !read_stormbolt_field(c.actor,0,table) || table!=api_.actor_vtable ||
            !read_stormbolt_field(c.actor,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(c.actor,0x76B0,current_weapon) || current_weapon!=c.weapon ||
            !read_stormbolt_field(c.weapon,0,table) || table!=api_.weapon_vtable ||
            !read_stormbolt_field(c.weapon,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(c.weapon,0x9B0,owner) || owner!=c.actor ||
            !read_stormbolt_field(c.weapon,0x9F8,category) || category!=13 ||
            !read_stormbolt_field(c.weapon,0x9FC,id) || (id!=238 && id!=239)) return false;
        if(!read_stormbolt_field(t,0x18,c.root) || !read_stormbolt_field(c.actor,0x14F8,root) || root!=c.root ||
            !stormbolt_native_span(root,0x10,0x8E0) ||
            !read_stormbolt_field(t,0x20,c.input) || c.input!=root+0x10 ||
            !read_stormbolt_field(c.input,0x30,input_actor) || input_actor!=c.actor ||
            !read_stormbolt_field(c.actor,0x12608,c.manager) ||
            !read_stormbolt_field(c.manager,0,table) || table!=api_.manager_vtable ||
            !read_stormbolt_field(c.manager,0xD90,owner) || owner!=c.actor ||
            !read_stormbolt_field(c.manager,0x40,input_actor) || input_actor!=c.actor ||
            !read_stormbolt_field(c.actor,0xE480,c.physical) || !stormbolt_native_span(c.physical,0,0x84) ||
            !stormbolt_native_span(c.actor,0xE44C,1) || !stormbolt_native_span(c.actor,0x8965,1) ||
            !read_stormbolt_field(c.actor,0x7810,c.parameter) || !stormbolt_native_span(c.weapon,0,0x2850)) return false;
        return true;
    }
    bool parameter(const Context& c,std::size_t offset,std::size_t size) const {
        std::uintptr_t table=0;
        return api_.parameter_vtable && read_stormbolt_field(c.parameter,0,table) && table==api_.parameter_vtable &&
            stormbolt_native_span(c.parameter,offset,size);
    }
    bool local_record(const Context& c) const {
        std::uintptr_t manager=0;
        if(!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(api_.player_manager_slot),0,manager) ||
            !stormbolt_native_span(manager,0,0xAE48)) return false;
        std::uint64_t key=0;
        if(!read_stormbolt_field(manager,0xAE40,key)) return false;
        for(unsigned row=0;row<24;++row) {
            const auto offset=row*0x740;
            std::int32_t a=0,b=0;
            if(!read_stormbolt_field(manager,offset+0x50,a) || !read_stormbolt_field(manager,offset+0x54,b)) return false;
            if(a<0 && b<0) continue;
            std::uint64_t candidate_key=0;
            if(!read_stormbolt_field(manager,offset+0x48,candidate_key)) return false;
            if(candidate_key!=key) continue;
            std::uintptr_t candidate=0;std::uint8_t alive=0;
            if(!read_stormbolt_field(manager,offset+0x58,candidate)) return false;
            if(!candidate) return true;
            if(!read_stormbolt_field(candidate,0xC,alive)) return false;
            return !(alive&0xE) || candidate!=c.actor || stormbolt_native_span(c.root,0x4EF8,4,true);
        }
        return true;
    }
public:
    StormboltConditionReadiness(StormboltInstanceStore<Capacity>& store,const StormboltConditionReadinessBindings& api,
        bool (*dispatch_ready)(void*)=nullptr,void* dispatch_context=nullptr)
        :store_(store),api_(api),dispatch_ready_(dispatch_ready),dispatch_context_(dispatch_context) {}
    bool shoot(void* transition,void* weapon) const {
        Context c;
        if(!common(transition,weapon,c)) return false;
        StormboltInstanceToken token{};StormboltStageState state{};
        if(!store_.token_for_live_address(c.weapon,token) || !store_.snapshot(token,state)) return false;
        return state.counter>=8 || parameter(c,0x1624+40*state.counter,1);
    }
    bool setbomb(void* transition,void* weapon) const {
        Context c;
        if(!common(transition,weapon,c)) return false;
        std::uintptr_t component=0,status=0,authority=0,system=0;
        std::uint32_t command=0;
        if(!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(transition),0x28,command) || command!=0x20 ||
            !stormbolt_native_span(reinterpret_cast<std::uintptr_t>(transition),0x30,1) ||
            !stormbolt_native_span(c.input,0x860,0x14,true) || !stormbolt_native_span(c.input,0x8A0,0x14,true) ||
            !read_stormbolt_field(c.actor,0x7658,component) || !stormbolt_native_span(component,0xA4,3) ||
            !read_stormbolt_field(c.actor,0x7D20,status) || !stormbolt_native_span(status,0x661,1) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(api_.authority_slot),0,authority) || !stormbolt_native_span(authority,0x38,1) ||
            !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(api_.command_system_slot),0,system) ||
            !stormbolt_native_span(system,0x146D5,1) || !stormbolt_native_span(system,0x146EA,1) ||
            !local_record(c)) return false;
        // Missing native parameter is a legitimate return3 path in the PS core.
        return !c.parameter || parameter(c,0x9C0,4);
    }
    static bool ready_shoot(void* transition,void* weapon,void* context) {
        return context && static_cast<StormboltConditionReadiness*>(context)->shoot(transition,weapon);
    }
    static bool ready_setbomb(void* transition,void* weapon,void* context) {
        return context && static_cast<StormboltConditionReadiness*>(context)->setbomb(transition,weapon);
    }
};
} // namespace stormslinger
