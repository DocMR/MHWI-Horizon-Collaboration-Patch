#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_evade_reload_effect.h"
#include "stormbolt_evade_reload_effect_session.h"
#include "stormbolt_evade_effect_readiness.h"
#include "stormbolt_runtime_callback_readiness.h"
#include "stormbolt_runtime_dispatch_gate.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>
using namespace stormslinger;
namespace {
StormboltInstanceStore<2> store;
std::array<unsigned char,0x2900> fixture_weapon{};
std::array<unsigned char,0x8900> actor{};
unsigned originals=0,effects=0,order=0;int mutation=0;
HANDLE callback_entered=nullptr,callback_resume=nullptr;
template<class T> void put(void* p,std::size_t off,T v) {std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
void original(void* p) {
    assert(p==fixture_weapon.data());++originals;order=1;
    if(callback_entered) {SetEvent(callback_entered);assert(WaitForSingleObject(callback_resume,5000)==WAIT_OBJECT_0);}
    if(mutation==1) store.initialize(reinterpret_cast<std::uintptr_t>(p));
    if(mutation==2) put(p,0x1D00,std::uint32_t{23});
}
bool ready(void* p,void* context) {
    assert(order==1 && context==actor.data());
    if(mutation==3) store.initialize(reinterpret_cast<std::uintptr_t>(p));
    return mutation!=4;
}
void effect(void* provider,std::uint32_t event,std::uint32_t record,std::uint32_t argument) {
    assert(order==1 && provider==actor.data()+16 && event==2000 && record==0 && argument==0xFFFFFFFF);
    order=2;++effects;
}
void reset() {
    fixture_weapon.fill(0);actor.fill(0);mutation=0;
    store.initialize(reinterpret_cast<std::uintptr_t>(fixture_weapon.data()));
    put(fixture_weapon.data(),0xC,std::uint8_t{2});put(actor.data(),0xC,std::uint8_t{2});
    put(fixture_weapon.data(),0x9F8,std::uint32_t{13});put(fixture_weapon.data(),0x9FC,std::uint32_t{239});
    put(fixture_weapon.data(),0x1D00,std::uint32_t{24});put(fixture_weapon.data(),0x9B0,actor.data());
    put(actor.data(),0x8808,actor.data()+16);
}
}
int main() {
    {
        constexpr std::size_t count=sizeof(stormbolt_weapon_effect_records)/sizeof(stormbolt_weapon_effect_records[0]);
        static_assert(count==17);
        std::array<unsigned char,0x1F8> resource{};
        std::array<std::array<unsigned char,0x18>,8> groups{};
        std::array<std::array<unsigned char,0x2C0>,count> records{};
        std::array<std::array<unsigned char,0x68>,count> effects_resources{};
        std::array<unsigned char,0x38> component{};
        put(resource.data(),0,std::uintptr_t{0xCAFE});put(resource.data(),0x64,std::uint32_t{1});
        put(resource.data(),0xB8,groups.data());put(resource.data(),0xC0,std::uint32_t{8});
        put(component.data(),0x30,resource.data());
        unsigned group_index=0;
        for(std::size_t i=0;i<count;++i) {
            const auto key=stormbolt_weapon_effect_records[i];
            if(i==0 || key.group!=stormbolt_weapon_effect_records[i-1].group) {
                if(i) ++group_index;
                std::uint32_t n=0;for(std::size_t j=i;j<count && stormbolt_weapon_effect_records[j].group==key.group;++j) ++n;
                put(groups[group_index].data(),8,records[i].data());put(groups[group_index].data(),0x10,n);
                put(groups[group_index].data(),0x14,key.group);
            }
            put(records[i].data(),0x2B0,key.record);put(records[i].data(),8,effects_resources[i].data());
            put(effects_resources[i].data(),0,std::uintptr_t{0xBEEF});put(effects_resources[i].data(),0x64,std::uint32_t{1});
        }
        StormboltWeaponEffectDependencies dependencies{0xCAFE,0xBEEF};
        auto validate=[&] {return StormboltWeaponEffectDependencies::callback(component.data(),resource.data(),2000,0,&dependencies);};
        const auto before=resource;assert(validate() && resource==before);
        for(std::size_t i=0;i<count;++i) {
            put(records[i].data(),8,std::uintptr_t{0});assert(!validate());put(records[i].data(),8,effects_resources[i].data());
            put(effects_resources[i].data(),0,std::uintptr_t{0xBAD});assert(!validate());put(effects_resources[i].data(),0,std::uintptr_t{0xBEEF});
            for(std::uint32_t flags:{0u,0x11u,0x41u}) {put(effects_resources[i].data(),0x64,flags);assert(!validate());}
            put(effects_resources[i].data(),0x64,std::uint32_t{1});
        }
        put(component.data(),0x30,std::uintptr_t{0});assert(!validate());put(component.data(),0x30,resource.data());
        assert(!StormboltWeaponEffectDependencies::callback(component.data(),resource.data(),2001,0,&dependencies));
        assert(validate());
        std::puts("PASS owned weapon dependency callback: all17 declared EFX slots, per-slot absence/type/publication/failure, component identity and no writes.");
    }
    {
        std::array<unsigned char,0x1F8> resource{};
        std::array<unsigned char,0x18> groups{};
        std::array<unsigned char,0x2C0> records{};
        const auto identity=reinterpret_cast<std::uintptr_t>(resource.data());
        put(resource.data(),0,identity);
        put(resource.data(),0xB8,reinterpret_cast<std::uintptr_t>(groups.data()));
        put(resource.data(),0xC0,std::uint32_t{1});
        put(groups.data(),8,reinterpret_cast<std::uintptr_t>(records.data()));
        put(groups.data(),0x10,std::uint32_t{1});
        put(groups.data(),0x14,std::uint16_t{2000});
        put(records.data(),0x2B0,std::uint16_t{0});
        const auto before=resource;const auto group_before=groups;const auto record_before=records;
        assert(stormbolt_epv_record(identity,identity,2000,0)==reinterpret_cast<std::uintptr_t>(records.data()+0x30));
        assert(!stormbolt_epv_record(identity,identity+8,2000,0));
        assert(!stormbolt_epv_record(identity,identity,2001,0));
        assert(!stormbolt_epv_record(identity,identity,2000,1));
        assert(!stormbolt_epv_record(identity,identity,0x10000,0));
        assert(resource==before && groups==group_before && records==record_before);
        put(groups.data(),0x10,std::uint32_t{0});assert(!stormbolt_epv_record(identity,identity,2000,0));
        put(resource.data(),0xC0,std::uint32_t{0});assert(!stormbolt_epv_record(identity,identity,2000,0));
        auto* denied=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(denied);
        assert(!stormbolt_epv_record(reinterpret_cast<std::uintptr_t>(denied),identity,2000,0));
        assert(VirtualFree(denied,0,MEM_RELEASE));
        std::puts("PASS EPV typed record view: real native layout, missing IDs/counts, noaccess and no writes.");
    }
    {
        reset();
        std::array<void*,1> weapon_type{},actor_type{},component_type{},dti_type{};
        std::array<void*,34> provider_type{};
        std::array<unsigned char,0x400> component{};
        std::array<unsigned char,0x40> provider_object{},provider_dti{};
        std::uint32_t resource_header=0xCAFE;
        put(fixture_weapon.data(),0,weapon_type.data());put(actor.data(),0,actor_type.data());
        put(actor.data(),0x76B0,fixture_weapon.data());put(actor.data(),0x8808,component.data());
        put(component.data(),0,component_type.data());put(component.data(),0x18,actor.data());
        put(component.data(),0x30,&resource_header);put(provider_object.data(),0,provider_type.data());
        put(provider_dti.data(),0,dti_type.data());
        auto create=+[](void*,void*,void*,std::uint32_t,std::uint32_t,void*,void*,void*)->void* {return nullptr;};
        provider_type[0x20/8]=reinterpret_cast<void*>(&original);
        provider_type[0xF8/8]=reinterpret_cast<void*>(&effect);provider_type[0x108/8]=reinterpret_cast<void*>(create);
        void* singleton=provider_object.data();
        struct Check {void* component;unsigned calls=0;bool allowed=true;} check{component.data()};
        StormboltEvadeEffectReadinessBindings config{
            reinterpret_cast<std::uintptr_t>(weapon_type.data()),reinterpret_cast<std::uintptr_t>(actor_type.data()),
            reinterpret_cast<std::uintptr_t>(component_type.data()),
            {&singleton,reinterpret_cast<std::uintptr_t>(provider_type.data()),reinterpret_cast<std::uintptr_t>(provider_dti.data()),
             reinterpret_cast<std::uintptr_t>(dti_type.data()),reinterpret_cast<std::uintptr_t>(&original),reinterpret_cast<std::uintptr_t>(&effect),create},
            true,+[](void* cp,void* resource,std::uint32_t group,std::uint32_t record,void* context) {
                auto& c=*static_cast<Check*>(context);++c.calls;std::uint32_t header=0;
                return c.allowed && cp==c.component && group==2000 && record==0 &&
                    read_stormbolt_field(reinterpret_cast<std::uintptr_t>(resource),0,header) && header==0xCAFE;
            },&check};
        auto validate=[&] {return StormboltEvadeEffectReadiness(config).ready(fixture_weapon.data());};
        assert(validate() && check.calls==1);
        {
            StormboltRuntimeDispatchGate dispatch;
            StormboltEvadeEffectReadiness gated(config,+[](void* p){return static_cast<StormboltRuntimeDispatchGate*>(p)->weapon();},&dispatch);
            assert(!gated.ready(fixture_weapon.data()) && check.calls==1);
            dispatch.enable();assert(gated.ready(fixture_weapon.data()) && check.calls==2);
            dispatch.begin_restore();assert(!gated.ready(fixture_weapon.data()) && check.calls==2);
            dispatch.finish_restore();assert(!gated.ready(fixture_weapon.data()) && check.calls==2);
            check.calls=1;
        }
        {
            std::array<unsigned char,0x1768> parameter{};
            std::array<unsigned char,0x198> event{};
            std::array<unsigned char,0x30> action{};
            std::array<void*,1> parameter_type{};
            void* manager=actor.data();
            put(actor.data(),0x7810,parameter.data());put(parameter.data(),0,parameter_type.data());
            put(actor.data(),0x100,event.data());put(action.data(),0x28,actor.data());
            StormboltNativeBindings native{};
            native.shell.weapon_vtable=config.weapon_vtable;native.normal_to_special.actor_vtable=config.actor_vtable;
            native.coefficients.parameter_vtable=reinterpret_cast<std::uintptr_t>(parameter_type.data());
            native.gatling_entry.player_manager_slot=&manager;
            native.special_to_normal.action.event_object=+[](void* owner)->void* {
                void* result=nullptr;read_stormbolt_field(reinterpret_cast<std::uintptr_t>(owner),0x100,result);return result;
            };
            native.special_to_normal.hook.action_vtable=0x1000;native.gatling_hook.action_vtable=0x1008;
            native.middle_hook.action_vtable=0x1010;native.last_hook.action_vtable=0x1018;
            native.primary_shoot_hook.action_vtable=0x1020;
            StormboltRuntimeCallbackReadiness ready(native,config,true),unborrowed(native,config,false);
            StormboltRuntimeDispatchGate dispatch;
            assert(!dispatch.weapon() && !dispatch.action(StormboltSpecialActionPhase::exit));
            const auto weapon_before=fixture_weapon;
            const auto actor_before=actor;
            assert(ready.weapon(fixture_weapon.data()) && !unborrowed.weapon(fixture_weapon.data()));
            unsigned i=0;
            for(auto kind:{StormboltRuntimeAction::special,StormboltRuntimeAction::gatling,StormboltRuntimeAction::middle,StormboltRuntimeAction::last,StormboltRuntimeAction::primary_shoot}) {
                dispatch.enable();assert(dispatch.weapon());
                put(action.data(),0,std::uintptr_t{0x1000+8*i++});
                assert(ready.action(action.data(),StormboltSpecialActionPhase::enter,kind));
                assert(ready.action(action.data(),StormboltSpecialActionPhase::update,kind));
                assert(!unborrowed.action(action.data(),StormboltSpecialActionPhase::exit,kind));
                check.allowed=false;
                assert(!ready.action(action.data(),StormboltSpecialActionPhase::update,kind));
                assert(ready.action(action.data(),StormboltSpecialActionPhase::exit,kind));check.allowed=true;
                dispatch.begin_restore();
                assert(!dispatch.weapon() && !dispatch.action(StormboltSpecialActionPhase::enter) &&
                    !dispatch.action(StormboltSpecialActionPhase::update));
                assert(dispatch.action(StormboltSpecialActionPhase::exit) && ready.action(action.data(),StormboltSpecialActionPhase::exit,kind));
                dispatch.finish_restore();assert(!dispatch.action(StormboltSpecialActionPhase::exit));
            }
            assert(fixture_weapon==weapon_before && actor==actor_before);
            put(parameter.data(),0,std::uintptr_t{0xBAD});assert(!ready.weapon(fixture_weapon.data()));put(parameter.data(),0,parameter_type.data());
            manager=nullptr;assert(!ready.weapon(fixture_weapon.data()));manager=actor.data();
            put(actor.data(),0x100,static_cast<void*>(nullptr));assert(!ready.weapon(fixture_weapon.data()));put(actor.data(),0x100,event.data());
            put(actor.data(),0x76B0,static_cast<void*>(nullptr));assert(!ready.weapon(fixture_weapon.data()));put(actor.data(),0x76B0,fixture_weapon.data());
            put(action.data(),0,std::uintptr_t{0xBAD});assert(!ready.action(action.data(),StormboltSpecialActionPhase::exit,StormboltRuntimeAction::last));
            assert(ready.weapon(fixture_weapon.data()));check.calls=1;
            std::puts("PASS runtime callbacks: all4 action families, owned checks, refused borrow/dependencies and exit cleanup without resources.");
            std::puts("PASS dispatch publication: dormant callbacks refused, enabled actions allowed, restoring only exits, stopped callbacks refused.");
        }
        {
            std::array<unsigned char,0x1F8> epv{};std::array<unsigned char,0x18> group{};
            std::array<unsigned char,0x2C0> record{};
            put(epv.data(),0,std::uintptr_t{0xCAFE});
            put(epv.data(),0x64,std::uint32_t{1});
            put(epv.data(),0xB8,reinterpret_cast<std::uintptr_t>(group.data()));
            put(epv.data(),0xC0,std::uint32_t{1});
            put(group.data(),8,reinterpret_cast<std::uintptr_t>(record.data()));
            put(group.data(),0x10,std::uint32_t{1});put(group.data(),0x14,std::uint16_t{2000});
            config.epv_resource_vtable=0xCAFE;put(component.data(),0x30,epv.data());
            assert(validate());
            const auto before_publish=check.calls;
            for(std::uint32_t flags:{0u,0x11u,0x41u}) {
                put(epv.data(),0x64,flags);assert(!validate() && check.calls==before_publish);
            }
            put(epv.data(),0x64,std::uint32_t{1});
            config.efx_resource_vtable=0xBEEF;
            const auto before_effect=check.calls;
            assert(!validate() && check.calls==before_effect);
            std::array<unsigned char,0xB0> effect_resource{};
            put(effect_resource.data(),0,std::uintptr_t{0xBEEF});
            put(effect_resource.data(),0x64,std::uint32_t{1});
            put(record.data(),8,reinterpret_cast<std::uintptr_t>(effect_resource.data()));
            assert(validate());
            put(effect_resource.data(),0,std::uintptr_t{0xBAD});const auto before_type=check.calls;
            assert(!validate() && check.calls==before_type);
            put(effect_resource.data(),0,std::uintptr_t{0xBEEF});
            for(std::uint32_t flags:{0u,0x11u,0x41u}) {
                put(effect_resource.data(),0x64,flags);assert(!validate() && check.calls==before_type);
            }
            put(effect_resource.data(),0x64,std::uint32_t{1});
            {
                config.mesh={1,2,3,4,5,6};
                const auto before_mesh=check.calls;
                assert(!validate() && check.calls==before_mesh);
                std::array<unsigned char,0x60> container{};
                std::array<unsigned char,0x40> first{},second{},mesh{},play_group{},play{};
                std::array<unsigned char,0x68> nested{};
                config.mesh.group=8;config.mesh.play_efx=9;
                std::uintptr_t group_pointer=reinterpret_cast<std::uintptr_t>(play_group.data());
                std::array<std::uintptr_t,2> play_parameters{{reinterpret_cast<std::uintptr_t>(play.data()),0}};
                put(container.data(),0x48,std::uint32_t{1});put(container.data(),0x58,&group_pointer);
                put(play_group.data(),0,std::uintptr_t{8});put(play_group.data(),0x20,std::uint32_t{2});
                put(play_group.data(),0x30,play_parameters.data());put(play.data(),0,std::uintptr_t{9});
                put(play.data(),0x30,nested.data());put(nested.data(),0,std::uintptr_t{0xBEEF});
                put(nested.data(),0x64,std::uint32_t{1});
                std::array<unsigned char,0x408> model{};
                std::array<unsigned char,0x68> positions{};
                std::array<unsigned char,0xB8> material{};
                std::array<unsigned char,0x28> material_data{};
                std::array<unsigned char,0x38> material_record{};
                std::array<unsigned char,0x25B8> material_manager{};
                std::uintptr_t manager_pointer=reinterpret_cast<std::uintptr_t>(material_manager.data());
                config.mesh.material_manager_slot=reinterpret_cast<std::uintptr_t>(&manager_pointer);
                unsigned char selected_material=0,fallback_material=0;
                std::uintptr_t source_material=reinterpret_cast<std::uintptr_t>(&selected_material);
                std::uintptr_t model_material=source_material;
                char material_name[]="Ch_Wp_Mt__3";
                put(model.data(),0xE4,std::uint32_t{1});put(model.data(),0x400,material_name);
                put(model.data(),0x3F8,&model_material);
                put(material.data(),0xA8,material_data.data());put(material.data(),0xB0,&source_material);
                put(material_data.data(),0x10,std::uint32_t{1});put(material_data.data(),0x20,material_record.data());
                put(material_record.data(),4,std::uint32_t{0x5B77893B});
                put(material_manager.data(),0x25B0,&fallback_material);
                config.mesh.texture=7;
                std::array<std::array<unsigned char,0x110>,9> texture_rows{};
                std::array<std::array<unsigned char,0x68>,9> textures{};
                put(material_data.data(),0x14,std::uint32_t{9});put(material_data.data(),0x18,texture_rows.data());
                for(unsigned i=0;i<9;++i) {
                    put(texture_rows[i].data(),0,std::uint32_t{0x241F5DEB});put(texture_rows[i].data(),8,textures[i].data());
                    put(textures[i].data(),0,std::uintptr_t{7});put(textures[i].data(),0x64,std::uint32_t{1});
                }
                std::array<std::uintptr_t,2> nodes{{reinterpret_cast<std::uintptr_t>(first.data()),reinterpret_cast<std::uintptr_t>(second.data())}};
                std::array<std::uintptr_t,7> first_parameters{};
                std::array<std::uintptr_t,9> second_parameters{};
                put(effect_resource.data(),0xA8,container.data());put(container.data(),0,std::uintptr_t{1});
                put(container.data(),0x28,std::uint32_t{2});put(container.data(),0x38,nodes.data());
                put(first.data(),0,std::uintptr_t{2});put(second.data(),0,std::uintptr_t{2});
                put(first.data(),0x20,std::uint32_t{7});put(second.data(),0x20,std::uint32_t{9});
                put(first.data(),0x30,first_parameters.data());put(second.data(),0x30,second_parameters.data());
                second_parameters[5]=reinterpret_cast<std::uintptr_t>(mesh.data());put(mesh.data(),0,std::uintptr_t{3});
                put(mesh.data(),0x30,model.data());put(mesh.data(),0x38,positions.data());
                put(model.data(),0,std::uintptr_t{4});put(positions.data(),0,std::uintptr_t{5});put(material.data(),0,std::uintptr_t{6});
                put(model.data(),0x3F0,material.data());
                for(auto* resource:{model.data(),positions.data(),material.data()}) put(resource,0x64,std::uint32_t{1});
                assert(validate());
                const auto before_missing=check.calls;
                for(auto* object:{container.data(),second.data(),mesh.data(),model.data(),positions.data(),material.data()}) {
                    std::uintptr_t saved=0;std::memcpy(&saved,object,sizeof(saved));
                    put(object,0,std::uintptr_t{0xBAD});assert(!validate() && check.calls==before_missing);
                    put(object,0,saved);
                }
                for(auto* resource:{model.data(),positions.data(),material.data()}) {
                    for(std::uint32_t flags:{0u,0x11u,0x41u}) {
                        put(resource,0x64,flags);assert(!validate() && check.calls==before_missing);
                    }
                    put(resource,0x64,std::uint32_t{1});
                }
                put(container.data(),0x28,std::uint32_t{1});assert(!validate());put(container.data(),0x28,std::uint32_t{2});
                put(second.data(),0x20,std::uint32_t{8});assert(!validate());put(second.data(),0x20,std::uint32_t{9});
                second_parameters[5]=0;assert(!validate());second_parameters[5]=reinterpret_cast<std::uintptr_t>(mesh.data());
                put(mesh.data(),0,std::uintptr_t{7});assert(!validate());put(mesh.data(),0,std::uintptr_t{3});
                put(mesh.data(),0x38,std::uintptr_t{0});assert(!validate());put(mesh.data(),0x38,positions.data());
                put(model.data(),0x3F0,std::uintptr_t{0});assert(!validate());put(model.data(),0x3F0,material.data());
                const auto before_selection=check.calls;
                model_material=reinterpret_cast<std::uintptr_t>(&fallback_material);assert(!validate());model_material=source_material;
                put(material_manager.data(),0x25B0,&selected_material);assert(!validate());put(material_manager.data(),0x25B0,&fallback_material);
                put(material_record.data(),4,std::uint32_t{0});assert(!validate());put(material_record.data(),4,std::uint32_t{0x5B77893B});
                material_name[0]='X';assert(!validate());material_name[0]='C';
                put(material_data.data(),0x10,std::uint32_t{0});assert(!validate());put(material_data.data(),0x10,std::uint32_t{1});
                manager_pointer=0;assert(!validate());manager_pointer=reinterpret_cast<std::uintptr_t>(material_manager.data());
                assert(check.calls==before_selection);
                for(unsigned i=0;i<9;++i) {
                    put(texture_rows[i].data(),8,std::uintptr_t{0});assert(!validate());put(texture_rows[i].data(),8,textures[i].data());
                    for(std::uint32_t flags:{0u,0x11u,0x41u}) {
                        put(textures[i].data(),0x64,flags);assert(!validate());
                    }
                    put(textures[i].data(),0x64,std::uint32_t{1});
                    put(textures[i].data(),0,std::uintptr_t{8});assert(!validate());put(textures[i].data(),0,std::uintptr_t{7});
                    put(texture_rows[i].data(),0,std::uint32_t{0});assert(!validate());put(texture_rows[i].data(),0,std::uint32_t{0x241F5DEB});
                }
                for(std::uint32_t count:{0u,8u,10u}) {put(material_data.data(),0x14,count);assert(!validate());}
                put(material_data.data(),0x14,std::uint32_t{9});
                assert(check.calls==before_selection);
                put(play.data(),0x30,std::uintptr_t{0});assert(!validate());
                put(play.data(),0x30,effect_resource.data());assert(!validate());put(play.data(),0x30,nested.data());
                for(std::uint32_t flags:{0u,0x11u,0x41u}) {put(nested.data(),0x64,flags);assert(!validate());}
                put(nested.data(),0x64,std::uint32_t{1});
                put(nested.data(),0,std::uintptr_t{0xBAD});assert(!validate());put(nested.data(),0,std::uintptr_t{0xBEEF});
                put(play_group.data(),0x20,std::uint32_t{1});assert(!validate());put(play_group.data(),0x20,std::uint32_t{2});
                put(play.data(),0,std::uintptr_t{0xBAD});assert(!validate());put(play.data(),0,std::uintptr_t{9});
                assert(check.calls==before_selection);
                assert(validate());check.allowed=false;assert(!validate());check.allowed=true;
                config.mesh={};
            }
            check.allowed=false;assert(!validate());check.allowed=true;
            const auto called=check.calls;
            put(record.data(),0x2B0,std::uint16_t{1});assert(!validate() && check.calls==called);
            config.epv_resource_vtable=0;config.efx_resource_vtable=0;
            put(component.data(),0x30,&resource_header);
            check.calls=1;
        }
        config.synchronous_borrow_verified=false;assert(!validate() && check.calls==1);config.synchronous_borrow_verified=true;
        put(actor.data(),0x76B0,static_cast<void*>(nullptr));assert(!validate());put(actor.data(),0x76B0,fixture_weapon.data());
        put(component.data(),0x18,static_cast<void*>(nullptr));assert(!validate());put(component.data(),0x18,actor.data());
        put(fixture_weapon.data(),0,actor_type.data());assert(!validate());put(fixture_weapon.data(),0,weapon_type.data());
        provider_type[0x108/8]=nullptr;assert(!validate());provider_type[0x108/8]=reinterpret_cast<void*>(create);
        singleton=nullptr;assert(!validate());singleton=provider_object.data();
        check.allowed=false;assert(!validate());check.allowed=true;
        auto* inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(inaccessible);
        put(actor.data(),0x8808,inaccessible);assert(!validate());put(actor.data(),0x8808,component.data());
        put(component.data(),0x30,inaccessible);assert(!validate());put(component.data(),0x30,&resource_header);
        assert(validate());assert(VirtualFree(inaccessible,0,MEM_RELEASE));
        config.resource_ready=nullptr;assert(!validate());
        std::puts("PASS typed evade readiness: reciprocal actor/weapon/component ownership, provider identity, explicit resource gate, noaccess and default borrow refusal.");
    }
    StormboltEvadeReloadEffectBindings api{original,effect,ready,actor.data(),0x1D00};
    StormboltEvadeReloadEffect<2> adapter(store,api);
    for(int id:{238,239,240}) for(int model:{23,24,25}) {
        reset();put(fixture_weapon.data(),0x9FC,static_cast<std::uint32_t>(id));
        put(fixture_weapon.data(),0x1D00,static_cast<std::uint32_t>(model));
        const auto before=fixture_weapon;const auto count=effects;const auto calls=originals;
        const bool expected=id!=240 && model==24;
        assert(adapter.invoke(fixture_weapon.data())==expected);
        assert(effects==count+static_cast<unsigned>(expected) && originals==calls+1 && fixture_weapon==before);
    }
    for(int change:{1,2,3,4}) {reset();mutation=change;const auto count=effects,calls=originals;
        assert(!adapter.invoke(fixture_weapon.data()) && effects==count && originals==calls+1);}
    reset();put(actor.data(),0xC,std::uint8_t{0});assert(!adapter.invoke(fixture_weapon.data()));
    reset();put(fixture_weapon.data(),0x9F8,std::uint32_t{12});assert(!adapter.invoke(fixture_weapon.data()));
    reset();StormboltInstanceToken token{};assert(store.token_for_live_address(reinterpret_cast<std::uintptr_t>(fixture_weapon.data()),token));
    assert(store.retire(token));const auto calls=originals;assert(!adapter.invoke(fixture_weapon.data()) && originals==calls+1);
    reset();api.ready=nullptr;StormboltEvadeReloadEffect<2> unavailable(store,api);
    assert(!unavailable.invoke(fixture_weapon.data()));
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(page);
    const unsigned char prefix[]{0x48,0x83,0xEC,0x28};const unsigned char suffix[]{0x48,0x83,0xC4,0x28,0xC3};
    std::memcpy(page,prefix,4);std::memcpy(page+9,suffix,5);
    page[0x100]=0x48;page[0x101]=0xB8;auto original_pointer=&original;
    std::memcpy(page+0x102,&original_pointer,8);page[0x10A]=0xFF;page[0x10B]=0xE0;
    std::array<unsigned char,5> expected{};
    assert(build_stormbolt_loop_call(reinterpret_cast<std::uintptr_t>(page+4),reinterpret_cast<std::uintptr_t>(page+0x100),expected));
    std::memcpy(page+4,expected.data(),5);DWORD old{};assert(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
    auto native_caller=reinterpret_cast<void(*)(void*)>(page);
    reset();auto before_effects=effects,before_originals=originals;native_caller(fixture_weapon.data());
    assert(originals==before_originals+1 && effects==before_effects);
    api={reinterpret_cast<void(*)(void*)>(page+0x100),effect,ready,actor.data(),0x1D00};
    StormboltEvadeReloadEffectSession<2> session;
    auto changed=expected;changed[1]^=1;
    assert(!session.prepare(store,reinterpret_cast<std::uintptr_t>(page+4),api,changed));
    assert(session.prepare(store,reinterpret_cast<std::uintptr_t>(page+4),api,expected));
    assert(session.activate());native_caller(fixture_weapon.data());
    assert(originals==before_originals+2 && effects==before_effects+1);
    std::array<unsigned char,5> installed{};std::memcpy(installed.data(),page+4,5);
    assert(VirtualProtect(page,4096,PAGE_EXECUTE_READWRITE,&old));page[4]=0xCC;
    assert(VirtualProtect(page,4096,old,&old));
    assert(!session.release_after_quiescence() && session.active());
    assert(!session.restore() && session.active());
    assert(VirtualProtect(page,4096,PAGE_EXECUTE_READWRITE,&old));std::memcpy(page+4,installed.data(),5);
    assert(VirtualProtect(page,4096,old,&old));assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
    assert(session.restore_patch() && session.active() && std::memcmp(page+4,expected.data(),5)==0);
    assert(session.release_after_quiescence() && !session.active());
    native_caller(fixture_weapon.data());assert(originals==before_originals+3 && effects==before_effects+1);
    callback_entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    callback_resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    auto retiring=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    auto retired=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    assert(callback_entered && callback_resume && retiring && retired);
    StormboltEvadeReloadEffectSession<2> leased;
    assert(leased.prepare(store,reinterpret_cast<std::uintptr_t>(page+4),api,expected) && leased.activate());
    std::thread caller([&]{native_caller(fixture_weapon.data());});
    assert(WaitForSingleObject(callback_entered,5000)==WAIT_OBJECT_0);
    // This fixture thread is blocked past the CALL, so the site itself is idle.
    assert(leased.restore_patch());
    std::thread retire([&]{SetEvent(retiring);assert(leased.release_after_quiescence());SetEvent(retired);});
    assert(WaitForSingleObject(retiring,5000)==WAIT_OBJECT_0);
    assert(WaitForSingleObject(retired,50)==WAIT_TIMEOUT);
    SetEvent(callback_resume);caller.join();
    assert(WaitForSingleObject(retired,5000)==WAIT_OBJECT_0);retire.join();
    assert(!leased.active());
    CloseHandle(callback_entered);CloseHandle(callback_resume);CloseHandle(retiring);CloseHandle(retired);
    callback_entered=callback_resume=nullptr;
    assert(VirtualFree(page,0,MEM_RELEASE));
    std::puts("PASS evade post-refill effect: original-first/once, resolved-model and target gates, owner alive, generation invalidation and missing readiness; fixture APIs, no game hook.");
    std::puts("PASS single CALL relay execution, changed-byte refusal, exact restore and conflict retention/retry; native executable fixture only.");
}
