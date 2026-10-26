#pragma once
#include "stormbolt_effect_provider_identity.h"

namespace stormslinger {
// pc-resource-publication.json: post-read publishes bit0; native acquisition
// rejects failure bits4/6. Caller must still hold the native lifetime borrow.
inline bool stormbolt_resource_published(std::uintptr_t resource) {
    std::uint32_t status=0;
    return read_stormbolt_field(resource,0x64,status) && (status&0x51)==1;
}
// rEffectProvider+B0: groups pointer+8/count+10, group stride18(hex),
// group records pointer+8/count+10/id+14; record stride2C0/id+2B0.
// Sources: pc-epv-reader-header.json and bowgun-native-epv-lookup-tests.json.
inline std::uintptr_t stormbolt_epv_record(std::uintptr_t resource,
    std::uintptr_t expected_vtable,std::uint32_t group,std::uint32_t record) {
    if(!resource || !expected_vtable || group>0xFFFF || record>0xFFFF) return 0;
    std::uintptr_t table=0,groups=0;std::uint32_t count=0;
    if(!read_stormbolt_field(resource,0,table) || table!=expected_vtable ||
        !read_stormbolt_field(resource,0xB8,groups) || !groups ||
        !read_stormbolt_field(resource,0xC0,count)) return 0;
    for(std::uint32_t i=0;i<count;++i) {
        const auto offset=static_cast<std::uintptr_t>(i)*0x18;
        if(groups>UINTPTR_MAX-offset) return 0;
        const auto entry=groups+offset;std::uint16_t id=0;
        if(!read_stormbolt_field(entry,0x14,id)) return 0;
        if(id!=group) continue;
        std::uintptr_t records=0;std::uint32_t n=0;
        if(!read_stormbolt_field(entry,8,records) || !records ||
            !read_stormbolt_field(entry,0x10,n)) return 0;
        for(std::uint32_t j=0;j<n;++j) {
            const auto delta=static_cast<std::uintptr_t>(j)*0x2C0;
            if(records>UINTPTR_MAX-delta) return 0;
            const auto item=records+delta;
            if(!read_stormbolt_field(item,0x2B0,id)) return 0;
            if(id==record) return item<=UINTPTR_MAX-0x30 ? item+0x30 : 0;
        }
        return 0;
    }
    return 0;
}
struct StormboltEvadeMeshIdentities {
    std::uintptr_t container=0,emitter=0,parameter=0,model=0,positions=0,material=0;
    std::uintptr_t material_manager_slot=0;
    std::uintptr_t texture=0;
    std::uintptr_t group=0,play_efx=0;
    bool configured() const {return container || emitter || parameter || model || positions || material || material_manager_slot || texture || group || play_efx;}
    bool complete() const {return container && emitter && parameter && model && positions && material && material_manager_slot && texture && group && play_efx;}
    bool operator==(const StormboltEvadeMeshIdentities& other) const {
        return container==other.container && emitter==other.emitter && parameter==other.parameter &&
            model==other.model && positions==other.positions && material==other.material &&
            material_manager_slot==other.material_manager_slot && texture==other.texture && group==other.group && play_efx==other.play_efx;
    }
};
struct StormboltWeaponEffectRecord {std::uint16_t group,record;};
// Exact nonempty first slots in the retained lbg024.epv3 (SHA911d85c9...).
inline constexpr StormboltWeaponEffectRecord stormbolt_weapon_effect_records[]={
    {100,0},{2000,0},{2010,0},{2010,1},{2010,2},{2010,3},
    {2011,0},{2011,1},{2011,2},{2011,3},{2012,0},{2012,1},{2012,2},{2012,3},
    {2013,0},{2030,0},{2100,0}
};
// Publication of every declared target EFX root. Native readers retain control
// of their child resources; this does not certify device/rendering acceptance.
struct StormboltWeaponEffectDependencies {
    std::uintptr_t epv_type=0,effect_type=0;
    bool ready(std::uintptr_t resource) const {
        if(!epv_type || !effect_type || !stormbolt_resource_published(resource)) return false;
        for(const auto& required:stormbolt_weapon_effect_records) {
            const auto data=stormbolt_epv_record(resource,epv_type,required.group,required.record);
            std::uintptr_t effect=0,table=0;
            if(data<0x28 || !read_stormbolt_field(data-0x28,0,effect) || !effect ||
                !read_stormbolt_field(effect,0,table) || table!=effect_type ||
                !stormbolt_resource_published(effect)) return false;
        }
        return true;
    }
    static bool callback(void* component,void* resource,std::uint32_t group,std::uint32_t record,void* context) {
        if(!context || group!=2000 || record!=0) return false;
        std::uintptr_t bound=0;
        return read_stormbolt_field(reinterpret_cast<std::uintptr_t>(component),0x30,bound) &&
            bound==reinterpret_cast<std::uintptr_t>(resource) &&
            static_cast<const StormboltWeaponEffectDependencies*>(context)->ready(bound);
    }
};
// This target model/MRL pair has one material, Ch_Wp_Mt__3 (native CRC5B77893B).
// Mirror the source selection in2383180/2262F10 without calling native loaders:
// match the descriptor, then require the model table to hold that source object.
inline bool stormbolt_evade_material_selected(std::uintptr_t model,std::uintptr_t material,
    std::uintptr_t manager_slot) {
    std::uintptr_t names=0,selected_table=0,data=0,records=0,sources=0,source=0,selected=0,manager=0,fallback=0;
    std::uint32_t count=0,hash=0;
    if(!read_stormbolt_field(model,0xE4,count) || count!=1 ||
        !read_stormbolt_field(model,0x400,names) || !names ||
        !read_stormbolt_field(model,0x3F8,selected_table) || !selected_table ||
        !read_stormbolt_field(material,0xA8,data) || !data ||
        !read_stormbolt_field(data,0x10,count) || count!=1 ||
        !read_stormbolt_field(data,0x20,records) || !records ||
        !read_stormbolt_field(records,4,hash) || hash!=0x5B77893B ||
        !read_stormbolt_field(material,0xB0,sources) || !sources ||
        !read_stormbolt_field(sources,0,source) || !source ||
        !read_stormbolt_field(selected_table,0,selected) || selected!=source ||
        !read_stormbolt_field(manager_slot,0,manager) || !manager ||
        !read_stormbolt_field(manager,0x25B0,fallback) || !fallback || source==fallback) return false;
    constexpr char expected[]="Ch_Wp_Mt__3";
    for(std::size_t i=0;i<sizeof(expected);++i) {
        char value=0;
        if(!read_stormbolt_field(names,i,value) || value!=expected[i]) return false;
    }
    return true;
}
// Native2383280 relocates data+18; each110-byte texture row stores its loaded
// resource at+8. All nine rows in this target MRL request rTexture241F5DEB.
inline bool stormbolt_evade_material_textures_loaded(std::uintptr_t material,std::uintptr_t texture_type) {
    std::uintptr_t data=0,rows=0;std::uint32_t count=0;
    if(!texture_type || !read_stormbolt_field(material,0xA8,data) || !data ||
        !read_stormbolt_field(data,0x14,count) || count!=9 ||
        !read_stormbolt_field(data,0x18,rows) || !rows) return false;
    for(std::uint32_t i=0;i<count;++i) {
        const auto offset=static_cast<std::uintptr_t>(i)*0x110;
        if(rows>UINTPTR_MAX-offset) return false;
        const auto row=rows+offset;
        std::uint32_t hash=0;std::uintptr_t texture=0,table=0;
        if(!read_stormbolt_field(row,0,hash) || hash!=0x241F5DEB ||
            !read_stormbolt_field(row,8,texture) || !texture ||
            !read_stormbolt_field(texture,0,table) || table!=texture_type ||
            !stormbolt_resource_published(texture)) return false;
    }
    return true;
}
// Target lbg024_001 has two emitters (7/9 parameters); the second emitter's
// sixth parameter is TypeMesh. Native2632170 appends parameters at node+30,
// with count+20;25CF4A8 stores nodes at container+38, count+28.
// This checks loaded links/publication and source selection, not GPU readiness.
inline bool stormbolt_evade_mesh_loaded(std::uintptr_t effect,const StormboltEvadeMeshIdentities& types,
    std::uintptr_t effect_type) {
    if(!types.complete()) return false;
    std::uintptr_t container=0,table=0,nodes=0,mesh=0;
    std::uint32_t count=0;
    if(!read_stormbolt_field(effect,0xA8,container) || !container ||
        !read_stormbolt_field(container,0,table) || table!=types.container ||
        !read_stormbolt_field(container,0x28,count) || count!=2 ||
        !read_stormbolt_field(container,0x38,nodes) || !nodes) return false;
    // The first preceding group has PlayEfx(cm_act_900), then PlayEmitter.
    // 25CF18B uses the same2632170 append layout as emitter parameters;
    // 26A8389 stores the loaded nested rEFX at PlayEfx+30.
    std::uintptr_t groups=0,group=0,group_parameters=0,play=0,nested=0;
    if(!effect_type || !read_stormbolt_field(container,0x48,count) || count!=1 ||
        !read_stormbolt_field(container,0x58,groups) || !groups ||
        !read_stormbolt_field(groups,0,group) || !group ||
        !read_stormbolt_field(group,0,table) || table!=types.group ||
        !read_stormbolt_field(group,0x20,count) || count!=2 ||
        !read_stormbolt_field(group,0x30,group_parameters) || !group_parameters ||
        !read_stormbolt_field(group_parameters,0,play) || !play ||
        !read_stormbolt_field(play,0,table) || table!=types.play_efx ||
        !read_stormbolt_field(play,0x30,nested) || !nested || nested==effect ||
        !read_stormbolt_field(nested,0,table) || table!=effect_type ||
        !stormbolt_resource_published(nested)) return false;
    for(unsigned i=0;i<2;++i) {
        std::uintptr_t node=0,parameters=0;
        if(!read_stormbolt_field(nodes,i*sizeof(std::uintptr_t),node) || !node ||
            !read_stormbolt_field(node,0,table) || table!=types.emitter ||
            !read_stormbolt_field(node,0x20,count) || count!=(i==0?7u:9u) ||
            !read_stormbolt_field(node,0x30,parameters) || !parameters) return false;
        if(i==1 && (!read_stormbolt_field(parameters,5*sizeof(std::uintptr_t),mesh) || !mesh)) return false;
    }
    if(!read_stormbolt_field(mesh,0,table) || table!=types.parameter) return false;
    std::uintptr_t model=0,positions=0,material=0;
    if(!read_stormbolt_field(mesh,0x30,model) || !model ||
        !read_stormbolt_field(mesh,0x38,positions) || !positions ||
        !read_stormbolt_field(model,0,table) || table!=types.model || !stormbolt_resource_published(model) ||
        !read_stormbolt_field(positions,0,table) || table!=types.positions || !stormbolt_resource_published(positions) ||
        !read_stormbolt_field(model,0x3F0,material) || !material ||
        !read_stormbolt_field(material,0,table) || table!=types.material || !stormbolt_resource_published(material)) return false;
    return stormbolt_evade_material_selected(model,material,types.material_manager_slot) &&
        stormbolt_evade_material_textures_loaded(material,types.texture);
}
struct StormboltEvadeEffectReadinessBindings {
    std::uintptr_t weapon_vtable=0,actor_vtable=0,component_vtable=0;
    StormboltEffectProviderIdentity provider{};
    bool synchronous_borrow_verified=false;
    bool (*resource_ready)(void* component,void* resource,std::uint32_t group,
        std::uint32_t record,void* context)=nullptr;
    void* resource_context=nullptr;
    // Optional during fixture migration; production bindings must supply the
    // authenticated rEffectProvider identity before using native EPV validation.
    std::uintptr_t epv_resource_vtable=0;
    std::uintptr_t efx_resource_vtable=0;
    StormboltEvadeMeshIdentities mesh{};
};
// Typed live links plus the existing provider identity snapshot. Resource/EPV
// validation is an explicit dependency, not inferred from readable pointers.
class StormboltEvadeEffectReadiness {
    StormboltEvadeEffectReadinessBindings api_;
    bool (*dispatch_ready_)(void*)=nullptr;
    void* dispatch_context_=nullptr;
public:
    explicit StormboltEvadeEffectReadiness(StormboltEvadeEffectReadinessBindings api,
        bool (*dispatch_ready)(void*)=nullptr,void* dispatch_context=nullptr)
        :api_(api),dispatch_ready_(dispatch_ready),dispatch_context_(dispatch_context) {}
    bool ready(void* weapon) const {
        if(dispatch_ready_ && !dispatch_ready_(dispatch_context_)) return false;
        if(!api_.synchronous_borrow_verified || !api_.weapon_vtable || !api_.actor_vtable ||
            !api_.component_vtable || !api_.resource_ready) return false;
        const auto address=reinterpret_cast<std::uintptr_t>(weapon);
        std::uintptr_t table=0,owner=0,current=0,component=0,backlink=0,resource=0;
        std::uint8_t alive=0;std::uint32_t category=0,id=0;
        if(!read_stormbolt_field(address,0,table) || table!=api_.weapon_vtable ||
            !read_stormbolt_field(address,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(address,0x9F8,category) || category!=13 ||
            !read_stormbolt_field(address,0x9FC,id) || (id!=238 && id!=239) ||
            !read_stormbolt_field(address,0x9B0,owner) ||
            !read_stormbolt_field(owner,0,table) || table!=api_.actor_vtable ||
            !read_stormbolt_field(owner,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(owner,0x76B0,current) || current!=address ||
            !read_stormbolt_field(owner,0x8808,component) ||
            !read_stormbolt_field(component,0,table) || table!=api_.component_vtable ||
            !read_stormbolt_field(component,0x18,backlink) || backlink!=owner ||
            !read_stormbolt_field(component,0x30,resource) || !resource) return false;
        StormboltEffectProviderSnapshot snapshot{};
        if(snapshot_stormbolt_effect_provider(api_.provider,snapshot)!=StormboltProviderStatus::matched) return false;
        if(api_.epv_resource_vtable) {
            if(!stormbolt_resource_published(resource)) return false;
            const auto data=stormbolt_epv_record(resource,api_.epv_resource_vtable,2000,0);
            if(!data) return false;
            // The retained2000/0 record has a required first EFX path.
            // Native EPV loader stores its rEFX at record+8 (data-28 hex).
            if(api_.efx_resource_vtable) {
                std::uintptr_t effect=0,effect_type=0;
                if(data<0x28 || !read_stormbolt_field(data-0x28,0,effect) || !effect ||
                    !read_stormbolt_field(effect,0,effect_type) || effect_type!=api_.efx_resource_vtable ||
                    !stormbolt_resource_published(effect))
                    return false;
                if(api_.mesh.configured() && !stormbolt_evade_mesh_loaded(effect,api_.mesh,api_.efx_resource_vtable)) return false;
            }
        }
        return api_.resource_ready(reinterpret_cast<void*>(component),reinterpret_cast<void*>(resource),
            2000,0,api_.resource_context);
    }
    static bool callback(void* weapon,void* context) {
        return context && static_cast<const StormboltEvadeEffectReadiness*>(context)->ready(weapon);
    }
};
}
