#pragma once
#include "stormbolt_source_transition_create.h"
#include "stormbolt_instance_store.h"
#include "owned_vtable_pointer.h"
#include "hooks.h"

namespace stormslinger {
struct StormboltSourceTransitionState {};
struct StormboltSourceTransitionLifecycleBindings {
    std::uintptr_t type_vtable=0; // Dedicated SetBomb DTI31C4A40, creation at+8.
    void* (*original_create)(void*)=nullptr; // F03CA0.
    void* (*original_place)(void*,void*)=nullptr; // F03D20.
    void* (*original_destroy)(void*,std::uint32_t)=nullptr; // F0AE50.
};

// Prepare in the shared transaction, activate the factory under quiescence,
// and restore the factory BEFORE stopping that transaction. Release only after
// native hooks are removed; all callbacks/dependencies must outlive restoration.
// Register observed successful construction only, never an address lookup.
// Native SetBomb allocation is already 0x38 bytes; sidecars are out of line.
template<std::size_t Capacity>
class StormboltSourceTransitionLifecycle {
public:
    using Store=StormboltInstanceStore<Capacity,StormboltSourceTransitionState>;
private:
    Store store_;
    StormboltSourceTransitionCreateApi api_{};
    StormboltSourceTransitionLifecycleBindings bindings_{};
    decltype(bindings_.original_place) place_=nullptr;
    decltype(bindings_.original_destroy) destroy_=nullptr;
    OwnedVtablePointer<> factory_;
    inline static StormboltSourceTransitionLifecycle* owner_=nullptr;

    void retire(void* object) {
        StormboltInstanceToken token{};
        if(store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(object),token))
            store_.retire(token);
    }
    static void* on_place(void* native_type,void* storage) {
        auto& self=*owner_;
        self.retire(storage);
        auto* result=self.place_(native_type,storage);
        if(native_type==self.api_.type && result && result==storage)
            self.store_.initialize(reinterpret_cast<std::uintptr_t>(result));
        return result; // Preserve all native placement fields and defaults.
    }
    static void* on_destroy(void* object,std::uint32_t flags) {
        auto& self=*owner_;
        self.retire(object);
        return self.destroy_(object,flags);
    }
    static void* on_create(void* native_type) {
        auto& self=*owner_;
        if(native_type!=self.api_.type) return self.bindings_.original_create(native_type);
        auto* object=stormbolt_create_source_transition(self.api_);
        if(object) self.store_.initialize(reinterpret_cast<std::uintptr_t>(object));
        return object;
    }
public:
    Store& instances() noexcept {return store_;}
    bool factory_active() const noexcept {return factory_.active();}
    template<class Transaction>
    MH_STATUS prepare(Transaction& transaction,const StormboltSourceTransitionCreateApi& api,
        const StormboltSourceTransitionLifecycleBindings& bindings) {
        if(owner_ || !api.type || !api.allocator_for || !api.allocate || !api.place ||
            !bindings.type_vtable || !bindings.original_create || !bindings.original_place ||
            !bindings.original_destroy || api.place!=bindings.original_place)
            return MH_ERROR_NOT_INITIALIZED;
        api_=api;bindings_=bindings;owner_=this;
        auto status=transaction.add(reinterpret_cast<void*>(bindings.original_place),
            reinterpret_cast<void*>(&on_place),reinterpret_cast<void**>(&place_));
        if(status!=MH_OK) return status;
        return transaction.add(reinterpret_cast<void*>(bindings.original_destroy),
            reinterpret_cast<void*>(&on_destroy),reinterpret_cast<void**>(&destroy_));
    }
    // Caller first commits the queued lifecycle callbacks. Otherwise creation
    // could publish an instance without observing later reconstruction/destruction.
    bool activate_factory() {
        if(owner_!=this || !place_ || !destroy_) return false;
        return factory_.install(reinterpret_cast<void*>(bindings_.type_vtable+8),
            reinterpret_cast<void*>(bindings_.original_create),reinterpret_cast<void*>(&on_create));
    }
    bool restore_factory() {return factory_.restore();}
    bool release_after_stop() {
        if(factory_.active()) return false;
        if(owner_==this) owner_=nullptr;
        return true;
    }
};
} // namespace stormslinger
