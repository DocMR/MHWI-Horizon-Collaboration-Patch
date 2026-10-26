#pragma once
#include "stormbolt_stage_update.h"

namespace stormslinger {
using StormboltWeaponUpdate=void (*)(void*);
enum class StormboltWeaponUpdateResult { unavailable, original_only, invalidated, extended, extension_failed };
// Hook-independent call ordering. Caller establishes native object lifetime,
// target identity and serialization; only lifecycle-created sidecars are used.
template<std::size_t Capacity>
StormboltWeaponUpdateResult stormbolt_update_with_original(StormboltInstanceStore<Capacity>& store,
    void* weapon,StormboltWeaponUpdate original,const StormboltStageUpdateApi& api,
    std::uint32_t base_color,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa,bool (*ready)(void*)=nullptr) {
    if(!original) return StormboltWeaponUpdateResult::unavailable;
    StormboltInstanceToken token{};
    const bool tracked=store.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token);
    original(weapon); // Outside sidecar lock. Native exceptions propagate; never replay.
    if(!tracked) return StormboltWeaponUpdateResult::original_only;
    StormboltStageState state{};
    if(!store.snapshot(token,state)) return StormboltWeaponUpdateResult::invalidated;
    if(ready && !ready(weapon)) return StormboltWeaponUpdateResult::original_only;
    if(!store.snapshot(token,state)) return StormboltWeaponUpdateResult::invalidated;
    void* actor{};
    std::memcpy(&actor,static_cast<const std::byte*>(weapon)+0x9B0,sizeof(actor));
    return stormbolt_run_stage_update(store,token,actor,api,base_color,low_hlsa,high_hlsa)
        ? StormboltWeaponUpdateResult::extended : StormboltWeaponUpdateResult::extension_failed;
}
} // namespace stormslinger
