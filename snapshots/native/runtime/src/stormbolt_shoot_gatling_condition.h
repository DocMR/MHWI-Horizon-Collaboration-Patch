#pragma once
#include "stormbolt_shoot_gatling_native_fields.h"
#include "stormbolt_instance_store.h"
#include "stormbolt_native_memory.h"

namespace stormslinger {
struct StormboltShootConditionBindings {
    void* type=nullptr; // Native cHumanTransitionBase DTI; anonymous owned instance.
    void* (*create)(void*)=nullptr;
    void (*initialize)(void*,void*)=nullptr;
    void* (*destroy)(void*,std::uint32_t)=nullptr;
    void** vtable=nullptr; // Nine authenticated native base slots.
    std::uintptr_t actor_vtable=0,weapon_vtable=0;
    bool (*ready)(void*,void*,void*)=nullptr;
    void* ready_context=nullptr;
    StormboltShootGatlingNativeApi fields{};
};

// Own one native28-byte base condition with a private instance vtable. Preserve
// native RTTI/context initialization and native heap destruction. No type-global
// vtable is edited and no fields are appended to the native allocation.
// Call destroy_after_detach before owner destruction and after registry restore.
// Native callback/creation/destruction are serialized by the caller's machine
// lifecycle; the lookup mutex does not establish native pointer lifetimes.
template<std::size_t WeaponCapacity,std::size_t Owners=24>
class StormboltShootGatlingCondition {
    StormboltInstanceStore<WeaponCapacity>& store_;
    StormboltShootConditionBindings api_{};
    std::array<void*,9> table_{};
    void* object_=nullptr;
    inline static std::array<StormboltShootGatlingCondition*,Owners> owners_{};
    inline static std::mutex mutex_;
    static StormboltShootGatlingCondition* find(void* object) {
        if(!object) return nullptr;
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto* owner:owners_) if(owner && owner->object_==object) return owner;
        return nullptr;
    }
    void unregister() {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto& owner:owners_) if(owner==this) owner=nullptr;
    }
    static void* destroy(void* object,std::uint32_t flags) {
        auto* self=find(object);
        if(!self) return object;
        self->object_=nullptr;self->unregister(); // Retire before native destructor.
        return self->api_.destroy(object,flags);
    }
    bool select(void* object,void*& weapon,StormboltInstanceToken& token) {
        std::uintptr_t actor=0,table=0;std::uint8_t alive=0;
        if(!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(object),0x10,actor) ||
            !read_stormbolt_field(actor,0,table) || table!=api_.actor_vtable ||
            !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
            !read_stormbolt_field(actor,0x76B0,weapon)) return false;
        if(!store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token)) return false;
        const auto address=reinterpret_cast<std::uintptr_t>(weapon);
        std::uintptr_t owner=0;std::uint32_t category=0,id=0;
        return read_stormbolt_field(address,0,table) && table==api_.weapon_vtable &&
            read_stormbolt_field(address,0xC,alive) && (alive&0xE) &&
            read_stormbolt_field(address,0x9B0,owner) && owner==actor &&
            read_stormbolt_field(address,0x9F8,category) && category==13 &&
            read_stormbolt_field(address,0x9FC,id) && (id==238 || id==239);
    }
    static std::int32_t condition(void* object) {
        auto* self=find(object);
        if(!self) return -1; // Unknown objects are rejected before any dereference.
        void* weapon{};StormboltInstanceToken token{};
        if(!self->select(object,weapon,token) || !self->api_.ready(object,weapon,self->api_.ready_context)) return -1;
        void* current_weapon{};StormboltInstanceToken current{};
        if(!self->select(object,current_weapon,current) || current_weapon!=weapon ||
            current.generation!=token.generation || current.address!=token.address) return -1;
        // This is an additional dedicated transition, not the native normal
        // shooting condition. Removing the mod must disable it even while
        // ammo40/charge/stage still contain the previous special-mode state.
        if(!stormbolt_source_count(weapon)) return -1;
        StormboltStageState state{};
        if(!self->store_.snapshot(token,state)) return -1;
        std::int32_t result=-1;
        if(!stormbolt_read_shoot_gatling_condition(object,weapon,state,self->api_.fields,result)) return -1;
        return self->store_.snapshot(token,state)?result:-1;
    }
public:
    explicit StormboltShootGatlingCondition(StormboltInstanceStore<WeaponCapacity>& store):store_(store) {}
    StormboltShootGatlingCondition(const StormboltShootGatlingCondition&)=delete;
    StormboltShootGatlingCondition& operator=(const StormboltShootGatlingCondition&)=delete;
    void* object() const noexcept {return object_;}
    bool create(void* actor,const StormboltShootConditionBindings& api) {
        if(object_ || !actor || !api.type || !api.create || !api.initialize || !api.destroy || !api.vtable ||
            !api.actor_vtable || !api.weapon_vtable || !api.ready || !api.fields.held_query ||
            !api.fields.physical_primary || !api.fields.parameter_vtable) return false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto slot=std::find(owners_.begin(),owners_.end(),nullptr);
            if(slot==owners_.end()) return false;
            *slot=this;
        }
        api_=api;object_=api.create(api.type);
        if(!object_) {unregister();return false;}
        if(stormbolt_source_field<void**>(object_,0)!=api.vtable) {
            destroy_after_detach();return false;
        }
        api.initialize(object_,actor);
        std::memcpy(table_.data(),api.vtable,sizeof(table_));
        table_[0]=reinterpret_cast<void*>(&destroy);
        table_[6]=reinterpret_cast<void*>(&condition);
        auto* private_table=table_.data();std::memcpy(object_,&private_table,sizeof(private_table));
        return true;
    }
    void destroy_after_detach() {
        if(!object_) return;
        auto* object=object_;object_=nullptr;unregister();
        std::memcpy(object,&api_.vtable,sizeof(api_.vtable));
        api_.destroy(object,1);
    }
    ~StormboltShootGatlingCondition() {destroy_after_detach();}
};
} // namespace stormslinger
