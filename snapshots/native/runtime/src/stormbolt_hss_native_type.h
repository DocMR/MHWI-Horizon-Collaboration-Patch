#pragma once
#include "stormbolt_hss_gui_controller.h"
#include "stormbolt_default_heap_readiness.h"
#include <new>

namespace stormslinger {
struct StormboltHssTypeBindings {
    void (*register_type)(void*,const char*,void*,std::size_t,unsigned,unsigned,unsigned){};
    void* parent_type{};
    const void* base_vtable{};
    const void* dti_vtable{};
    void* (*construct_base)(void*){};
    void (*destroy_base)(void*){};
    void (*set_priority)(void*,unsigned){};
    void (*request_show)(void*,void*){};
    void (*remove_gui)(void*){}; // PC2228F10; native deferred removal
    void* (*local_actor)(){};
    StormboltDefaultHeapBindings heap{};
    StormboltHssGuiBindings gui{};
};
// Concrete native uGUIHudHSSLockOn type. The owner must remain process-resident:
// native DTI registration links metadata into global lists without an unlinking
// destructor. Session stop retires GUI instances/catalog publication separately.
template<std::size_t Capacity>
class StormboltHssNativeType {
    struct alignas(16) Object {
        unsigned char base[0x28F0];
        StormboltHssGuiController controller;
        Object* next{};
        Object* previous{};
        bool retiring{};
        unsigned char alignment_padding[7]{}; // Explicit 16-byte extent after localized-text bindings.
    };
    static_assert(offsetof(Object,controller)==0x28F0);
    alignas(8) unsigned char type_[0x38]{};
    std::array<void*,0x2E0/8> methods_{};
    std::array<void*,5> factories_{};
    StormboltHssTypeBindings api_{};
    StormboltInstanceStore<Capacity>* store_{};
    bool registered_{};
    Object* live_{};
    inline static StormboltHssNativeType* active_{};
    static Object* object(void* p){return static_cast<Object*>(p);}
    static void* place(void*,void* memory) {
        if(!memory) return nullptr;
        auto& self=*active_;
        self.api_.construct_base(memory);
        auto* p=object(memory);
        new(&p->controller) StormboltHssGuiController;
        p->previous=nullptr;p->next=self.live_;p->retiring=false;
        if(self.live_)self.live_->previous=p;
        self.live_=p;
        std::memcpy(memory,&self.method_pointer_,sizeof(void*));
        auto* flags=reinterpret_cast<std::uint32_t*>(p->base+0x1D60);
        flags[0]=(flags[0]&0xCAD12027u)|0x352CDFD8u;
        flags[1]|=0x19C00u;
        self.api_.set_priority(memory,5);
        return memory;
    }
    void* method_pointer_{};
    static void* create(void*) {
        auto& self=*active_;StormboltDefaultHeapSelection selected{};
        if(!stormbolt_select_default_heap(self.api_.heap,self.type_,selected)) return nullptr;
        std::uint32_t tag{};std::memcpy(&tag,self.type_+0x34,sizeof(tag));
        const char* label{};std::memcpy(&label,self.type_+8,sizeof(label));
        auto* memory=stormbolt_allocate_default_heap(self.api_.heap,self.type_,selected.heap,
            sizeof(Object),16,tag,0,0,label);
        return place(self.type_,memory);
    }
    static void* array_place(void* type,void* memory,std::uint32_t count) {
        if(!memory) return nullptr;
        auto* bytes=static_cast<unsigned char*>(memory);
        for(std::uint32_t i=0;i<count;++i)place(type,bytes+std::size_t(i)*sizeof(Object));
        return memory;
    }
    static void* destroy(void* memory,std::uint32_t flags) {
        auto& self=*active_;
        auto* p=object(memory);
        if(p->previous)p->previous->next=p->next;else self.live_=p->next;
        if(p->next)p->next->previous=p->previous;
        object(memory)->controller.retire();
        object(memory)->controller.~StormboltHssGuiController();
        self.api_.destroy_base(memory);
        if((flags&1) && !(flags&4)) {
            StormboltDefaultHeapSelection selected{};
            if(stormbolt_select_default_heap(self.api_.heap,self.type_,selected))
                reinterpret_cast<void(*)(void*,void*)>(selected.release)(selected.heap,memory);
        }
        return memory;
    }
    static void* type_of(void*){return active_->type_;}
    static const char* resource(void*){return "ui\\hud\\hud_hss";}
    static void initialize(void* gui) {
        auto& self=*active_;
        if(!object(gui)->controller.initialize(gui,self.api_.gui))
            self.api_.gui.hide_gui(gui,true);
    }
    static void update(void* gui) {
        auto& self=*active_;
        object(gui)->controller.update(gui,reinterpret_cast<std::uintptr_t>(self.api_.local_actor()),
            *self.store_);
    }
    static void display(void* gui) {
        auto& self=*active_;StormboltHssNativeInput input{};
        if(stormbolt_hss_native_input(reinterpret_cast<std::uintptr_t>(self.api_.local_actor()),
            self.api_.gui.identity,self.api_.gui.input,input) && input.visible)
            self.api_.request_show(gui,nullptr);
    }
    static void refresh_stage(void* gui) {
        auto& self=*active_;
        object(gui)->controller.refresh_stage(gui,
            reinterpret_cast<std::uintptr_t>(self.api_.local_actor()),*self.store_);
    }
    static void show(void* gui) {
        auto& self=*active_;
        object(gui)->controller.show(reinterpret_cast<std::uintptr_t>(self.api_.local_actor()),
            *self.store_);
    }
public:
    bool register_type(const StormboltHssTypeBindings& api,StormboltInstanceStore<Capacity>& store) {
        if(registered_ || active_ || !api.register_type || !api.parent_type ||
            !api.base_vtable || !api.dti_vtable || !api.construct_base || !api.destroy_base ||
            !api.set_priority || !api.request_show || !api.remove_gui || !api.local_actor) return false;
        api_=api;store_=&store;
        std::memcpy(methods_.data(),api.base_vtable,sizeof(methods_));
        std::memcpy(factories_.data(),api.dti_vtable,sizeof(factories_));
        methods_[0]=reinterpret_cast<void*>(&destroy);
        methods_[0x20/8]=reinterpret_cast<void*>(&type_of);
        methods_[0x238/8]=reinterpret_cast<void*>(&resource);
        methods_[0x250/8]=reinterpret_cast<void*>(&initialize);
        methods_[0x268/8]=reinterpret_cast<void*>(&update);
        methods_[0x278/8]=reinterpret_cast<void*>(&display);
        for(auto slot:{0x280u,0x288u,0x2A0u})methods_[slot/8]=reinterpret_cast<void*>(&refresh_stage);
        methods_[0x2D0/8]=reinterpret_cast<void*>(&show);
        factories_[1]=reinterpret_cast<void*>(&create);
        factories_[2]=reinterpret_cast<void*>(&place);
        factories_[3]=reinterpret_cast<void*>(&array_place);
        factories_[4]=reinterpret_cast<void*>(&destroy);
        method_pointer_=methods_.data();active_=this;
        api.register_type(type_,"uGUIHudHSSLockOn",api.parent_type,sizeof(Object),0,0,0);
        auto* factory_pointer=factories_.data();
        std::memcpy(type_,&factory_pointer,sizeof(factory_pointer));
        registered_=true;
        return true;
    }
    void* type() noexcept{return registered_?type_:nullptr;}
    // Native GUI lifetime/dispatch must be quiescent, as for the host's other
    // restore callbacks. Removal can synchronously destroy an instance, so no
    // object pointer is used after calling the native remover.
    void retire_instances() {
        for(;;) {
            auto* p=live_;
            while(p && p->retiring)p=p->next;
            if(!p)return;
            p->retiring=true;
            api_.gui.hide_gui(p,true);p->controller.retire();
            auto* flags=reinterpret_cast<std::uint32_t*>(p->base+0x14);
            *flags&=~8u; // Match PC1AE018A before native GUI removal.
            api_.remove_gui(p);
        }
    }
};
} // namespace stormslinger
