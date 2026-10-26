#pragma once
#include "stormbolt_power_cell_store.h"
#include <utility>

namespace stormslinger {
struct StormboltPowerCellConstruction {
    void* native_result;
    StormboltPowerCellToken token;
};
// Lifecycle adapter only. Hook caller authenticates types, native ABI, borrowed
// lifetime and serialized construction/reset/destruction. No native invocation
// occurs under the sidecar mutex; exceptions propagate without replay.
template<std::size_t Capacity>
class StormboltPowerCellLifecycle {
    StormboltPowerCellStore<Capacity>& store_;
    bool retire_at(std::uintptr_t address) {
        StormboltPowerCellToken token{};
        return store_.token_for_live_address(address,token) && store_.retire(token);
    }
public:
    explicit StormboltPowerCellLifecycle(StormboltPowerCellStore<Capacity>& store):store_(store) {}
    template<class Native, class Authenticate>
    StormboltPowerCellConstruction construct(void* component, Native&& native, Authenticate&& authenticate) {
        const auto address=reinterpret_cast<std::uintptr_t>(component);
        retire_at(address); // Reinitialization invalidates state before native writes.
        void* result=std::forward<Native>(native)();
        if (!component || result!=component || !std::forward<Authenticate>(authenticate)(component))
            return {result,{}};
        return {result,store_.initialize(address)};
    }
    template<class Native, class Authenticate>
    StormboltPowerCellToken reset(void* component, Native&& native, Authenticate&& authenticate) {
        const auto address=reinterpret_cast<std::uintptr_t>(component);
        const bool tracked=retire_at(address);
        std::forward<Native>(native)();
        // Constructor-internal reset sees no tracked state and must not create it.
        if (!tracked || !component || !std::forward<Authenticate>(authenticate)(component)) return {};
        return store_.initialize(address);
    }
    template<class Native>
    decltype(auto) destroy(void* component, Native&& native) {
        retire_at(reinterpret_cast<std::uintptr_t>(component));
        return std::forward<Native>(native)();
    }
};
} // namespace stormslinger
