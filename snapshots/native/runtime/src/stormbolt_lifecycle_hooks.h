#pragma once
#include "hooks.h"
#include "stormbolt_instance_store.h"
#include <atomic>

namespace stormslinger {
// PC LBG DTI allocation2147720, placement21477D0, virtual0 destructor21478F0.
// Bootstrap supplies these only after the executable and live code guard pass.
struct StormboltLifecycleEndpoints {
    void* allocate = nullptr;
    void* place = nullptr;
    void* destroy = nullptr;
    void* array_place = nullptr;
    std::size_t array_stride = 0;
};

// Process-lifetime object, shared with the shooting bridge. Keep its DLL pinned.
// This observes actual constructor/destructor boundaries; it does not keep a
// native object alive or implement equipment/action-specific reset conditions.
// No hook activation is performed here: the caller owns the common transaction.
template<std::size_t Capacity, class State=StormboltStageState,
         class Token=StormboltInstanceToken>
class StormboltLifecycleHooks {
public:
    using Allocate = void* (*)(void* type);
    using Construct = void* (*)(void* storage);
    using Place = void* (*)(void* type, void* storage);
    using Destroy = void* (*)(void* object, std::uint32_t flags);
    using ArrayPlace = void* (*)(void* type, void* storage, std::uint32_t count);
    struct Diagnostics {
        std::atomic<std::uint64_t> constructed{0}, retired{0},
            tracking_failures{0}, untracked_destructions{0};
    } diagnostics;

private:
    StormboltInstanceStore<Capacity,State,Token>& store_;
    Allocate allocate_ = nullptr;
    Construct construct_ = nullptr;
    Place place_ = nullptr;
    Destroy destroy_ = nullptr;
    ArrayPlace array_place_ = nullptr;
    std::size_t array_stride_ = 0;
    inline static StormboltLifecycleHooks* active_ = nullptr;

    void constructed(void* object) noexcept {
        if (!object) return; // Failed native allocation has no instance.
        try {
            if (store_.initialize(reinterpret_cast<std::uintptr_t>(object))) {
                ++diagnostics.constructed;
                return;
            }
        } catch (...) { }
        // Preserve the original return even if tracking fails. The shot bridge
        // must reject missing state, never lazily attach a new generation.
        ++diagnostics.tracking_failures;
    }
    void retiring(void* object) noexcept {
        try {
            Token token{};
            if (store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(object), token)) {
                if (store_.retire(token)) { ++diagnostics.retired; return; }
                ++diagnostics.tracking_failures;
                return;
            }
        } catch (...) { ++diagnostics.tracking_failures; return; }
        ++diagnostics.untracked_destructions;
    }
    static void* allocate_hook(void* type) {
        auto& hook_state = *active_;
        void* result = hook_state.allocate_(type);
        hook_state.constructed(result); // Original has fully initialized its result.
        return result;
    }
    static void* construct_hook(void* storage) {
        auto& state=*active_;
        state.retiring(storage);
        void* result=state.construct_(storage);
        if (result==storage) state.constructed(result);
        return result;
    }
    static void* place_hook(void* type, void* storage) {
        auto& hook_state = *active_;
        // Reconstruction ends the previous generation before native code can
        // re-enter a callback and observe partially rebuilt object fields.
        Token previous{};
        if(hook_state.store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(storage),previous))
            hook_state.retiring(storage);
        void* result = hook_state.place_(type, storage);
        hook_state.constructed(result);
        return result;
    }
    static void* array_place_hook(void* type,void* storage,std::uint32_t count) {
        auto& self=*active_;
        const auto address=reinterpret_cast<std::uintptr_t>(storage);
        const bool track=address && count && self.array_stride_ &&
            count<=(UINTPTR_MAX-address)/self.array_stride_;
        if(track) for(std::uint32_t n=0;n<count;++n) {
            auto* object=reinterpret_cast<void*>(address+n*self.array_stride_);
            Token token{};
            if(self.store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(object),token))
                self.retiring(object);
        }
        void* result=self.array_place_(type,storage,count);
        if(track && result==storage) for(std::uint32_t n=0;n<count;++n)
            self.constructed(reinterpret_cast<void*>(address+n*self.array_stride_));
        return result;
    }
    static void* destroy_hook(void* object, std::uint32_t flags) {
        auto& hook_state = *active_;
        hook_state.retiring(object); // Invalidate before original releases/frees it.
        return hook_state.destroy_(object, flags);
    }

public:
    explicit StormboltLifecycleHooks(StormboltInstanceStore<Capacity,State,Token>& instance_store)
        : store_(instance_store) { }
    StormboltLifecycleHooks(const StormboltLifecycleHooks&) = delete;
    StormboltLifecycleHooks& operator=(const StormboltLifecycleHooks&) = delete;
    bool owns_bridge() const noexcept {return active_==this;}
    // Only after shared transaction stop and native callback completion.
    bool release_after_unhook() {
        if (active_!=this) return false;
        active_=nullptr;allocate_=nullptr;place_=nullptr;construct_=nullptr;destroy_=nullptr;array_place_=nullptr;array_stride_=0;
        return true;
    }

    // Shared native ctor covers allocation, placement and array placement.
    // Use instead of add_to_transaction when all constructors converge here.
    MH_STATUS add_constructor_to_transaction(HookTransaction& transaction,
                                             void* construct,void* destroy) {
        if (active_) return MH_ERROR_ALREADY_CREATED;
        if (!construct || !destroy) return MH_ERROR_NOT_EXECUTABLE;
        active_=this;
        auto result=transaction.add(construct,reinterpret_cast<void*>(&construct_hook),
                                   reinterpret_cast<void**>(&construct_));
        if (result==MH_OK)
            result=transaction.add(destroy,reinterpret_cast<void*>(&destroy_hook),
                                   reinterpret_cast<void**>(&destroy_));
        return result;
    }

    MH_STATUS add_to_transaction(HookTransaction& transaction,
                                const StormboltLifecycleEndpoints& endpoints) {
        if (active_) return MH_ERROR_ALREADY_CREATED;
        if (!endpoints.allocate || !endpoints.place || !endpoints.destroy ||
            (endpoints.array_place && !endpoints.array_stride))
            return MH_ERROR_NOT_EXECUTABLE;
        // Bootstrap and teardown must be quiescent. Publish before activation;
        // retain this pointer after any failure, including failed rollback, so
        // an in-flight native callback can never target discarded bridge state.
        active_ = this;
        auto result = transaction.add(endpoints.allocate,
            reinterpret_cast<void*>(&allocate_hook), reinterpret_cast<void**>(&allocate_));
        if (result == MH_OK) result = transaction.add(endpoints.place,
            reinterpret_cast<void*>(&place_hook), reinterpret_cast<void**>(&place_));
        if (result == MH_OK) result = transaction.add(endpoints.destroy,
            reinterpret_cast<void*>(&destroy_hook), reinterpret_cast<void**>(&destroy_));
        if(result==MH_OK && endpoints.array_place) {
            array_stride_=endpoints.array_stride;
            result=transaction.add(endpoints.array_place,reinterpret_cast<void*>(&array_place_hook),
                reinterpret_cast<void**>(&array_place_));
        }
        return result;
    }
};
} // namespace stormslinger
