#pragma once
#include "stormbolt_source_manager_attachment.h"
#include "hooks.h"

namespace stormslinger {
struct StormboltSourceManagerLifecycleBindings {
    void (*initialize)(void*)=nullptr;
    void (*rebuild)(void*)=nullptr;
    void (*tick)(void*)=nullptr;
    void* (*destroy)(void*,std::uint32_t)=nullptr;
};

// All native calls run outside bookkeeping operations. Caller serializes each
// manager lifecycle and quiesces callbacks for restore_all/shared stop/release.
template<std::size_t WeaponCapacity,std::size_t Managers=24>
class StormboltSourceManagerLifecycle {
    using Attachment=StormboltSourceManagerAttachment<WeaponCapacity>;
    struct Entry {
        void* manager=nullptr;
        std::unique_ptr<Attachment> attachment;
        unsigned initialize_depth=0;
    };
    StormboltInstanceStore<WeaponCapacity>& store_;
    StormboltSourceManagerBindings api_{};
    StormboltSourceManagerLifecycleBindings original_{};
    std::array<Entry,Managers> entries_{};
    bool draining_=false;
    inline static StormboltSourceManagerLifecycle* owner_=nullptr;
    Entry* find(void* manager,bool create=false) {
        if(!manager) return nullptr;
        for(auto& entry:entries_) if(entry.manager==manager) return &entry;
        if(create && !draining_) for(auto& entry:entries_) if(!entry.manager) {
            try {entry.attachment=std::make_unique<Attachment>(store_);}
            catch(const std::bad_alloc&) {return nullptr;}
            entry.manager=manager;return &entry;
        }
        return nullptr;
    }
    bool detach(Entry* entry) {
        if(!entry || entry->attachment->before_rebuild_or_destroy()) return true;
        ++blocked_native_calls;return false;
    }
    void attach(Entry* entry) {
        if(!entry || draining_ || entry->initialize_depth || entry->attachment->active()) return;
        try {entry->attachment->after_rebuild(entry->manager,api_);}
        catch(const std::bad_alloc&) {entry->attachment->before_rebuild_or_destroy();}
    }
    static void initialize(void* manager) {
        auto& self=*owner_;auto* entry=self.find(manager,true);
        if(!self.detach(entry)) return;
        if(entry) ++entry->initialize_depth;
        self.original_.initialize(manager);
        if(entry) --entry->initialize_depth;
        self.attach(entry);
    }
    static void rebuild(void* manager) {
        auto& self=*owner_;auto* entry=self.find(manager,true);
        if(!self.detach(entry)) return;
        self.original_.rebuild(manager);self.attach(entry);
    }
    static void tick(void* manager) {
        auto& self=*owner_;auto* entry=self.find(manager,true);
        if(entry && entry->attachment->active() && !entry->attachment->matches_current_weapon())
            if(!self.detach(entry)) return;
        self.attach(entry); // Deferred initial attachment, before native dispatch.
        self.original_.tick(manager);
    }
    static void* destroy(void* manager,std::uint32_t flags) {
        auto& self=*owner_;auto* entry=self.find(manager);
        if(!self.detach(entry)) return manager;
        auto* result=self.original_.destroy(manager,flags);
        if(entry) *entry=Entry{};
        return result;
    }
public:
    unsigned blocked_native_calls=0;
    explicit StormboltSourceManagerLifecycle(StormboltInstanceStore<WeaponCapacity>& store):store_(store) {}
    bool active_for(void* manager) {
        auto* entry=find(manager);return entry && entry->attachment->active();
    }
    template<class Transaction>
    MH_STATUS prepare(Transaction& transaction,const StormboltSourceManagerLifecycleBindings& bindings,
        const StormboltSourceManagerBindings& api) {
        if(owner_ || !bindings.initialize || !bindings.rebuild || !bindings.tick || !bindings.destroy ||
            !api.manager_vtable || !api.condition.ready) return MH_ERROR_NOT_INITIALIZED;
        owner_=this;api_=api;draining_=false;
        auto status=transaction.add(reinterpret_cast<void*>(bindings.initialize),reinterpret_cast<void*>(&initialize),reinterpret_cast<void**>(&original_.initialize));
        if(status!=MH_OK) return status;
        status=transaction.add(reinterpret_cast<void*>(bindings.rebuild),reinterpret_cast<void*>(&rebuild),reinterpret_cast<void**>(&original_.rebuild));
        if(status!=MH_OK) return status;
        status=transaction.add(reinterpret_cast<void*>(bindings.tick),reinterpret_cast<void*>(&tick),reinterpret_cast<void**>(&original_.tick));
        if(status!=MH_OK) return status;
        return transaction.add(reinterpret_cast<void*>(bindings.destroy),reinterpret_cast<void*>(&destroy),reinterpret_cast<void**>(&original_.destroy));
    }
    bool restore_all() {
        draining_=true;
        for(auto& entry:entries_) if(entry.manager && !detach(&entry)) return false;
        return true;
    }
    // Shared transaction must already have stopped all four callbacks.
    bool release_after_stop() {
        for(const auto& entry:entries_) if(entry.manager && entry.attachment->active()) return false;
        for(auto& entry:entries_) entry=Entry{};
        if(owner_==this) owner_=nullptr;
        return true;
    }
};
} // namespace stormslinger
