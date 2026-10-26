#pragma once
#include "stormbolt_instance_store.h"
#include "stormbolt_shot_core.h"
#include <cstddef>
#include <cstring>

namespace stormslinger {
static_assert(sizeof(void*) == 8, "Native shell records require a 64-bit process");
// Native Windows x64 ABI: rShellParamList::virtual88 and wrapper1AA74A0.
// Bootstrap must validate binary/function identity before supplying these calls.
// Using the wrapper retains optional-owner callbacks, context copy, reference
// ownership and native scene registration. Never call virtual28/220 here.
struct StormboltShellCalls {
    const void* (*record_at)(void* list, std::uint32_t index) = nullptr;
    void* (*create_wrapped)(void* descriptor, void* weapon, void* owner,
                           const void* context) = nullptr;
};

struct StormboltBorrowedShot {
    void* weapon = nullptr;
    void* shell_list = nullptr; // Verified rShellParamList held at weapon+1D90.
    void* owner = nullptr;      // Preserve actual native R8, not an inferred player.
    const void* context = nullptr;
    std::uint32_t category = 0, equipment_id = 0, ammo = 0;
};

enum class StormboltShellStatus {
    unrelated, invalid_binding, stale_instance, missing_record,
    missing_descriptor, creation_failed, created
};
struct StormboltShellResult {
    StormboltShellStatus status = StormboltShellStatus::invalid_binding;
    void* shell = nullptr;
    std::uint32_t selector = 0;
};

// Integration seam, not an installed hook or an arbitrary-pointer API.
// Caller must establish the shot branch, weapon identity, live typed objects,
// game-thread/lifetime exclusion and EFX readiness before entry. The sidecar
// mutex cannot establish native lifetime. All borrowed objects and the returned
// 16-byte record must stay valid throughout this synchronous operation.
// Snapshot/advance and coefficient callback ordering belong to the shot caller;
// this function neither advances/clamps the stage nor rolls back native failure.
template<std::size_t Capacity>
inline StormboltShellResult create_stormbolt_shell(
    StormboltInstanceStore<Capacity>& store, StormboltInstanceToken token,
    const StormboltBorrowedShot& shot, const StormboltShellCalls& calls) {
    if (shot.category != 13 || (shot.equipment_id != 238 && shot.equipment_id != 239)
        || shot.ammo != 40)
        return {StormboltShellStatus::unrelated};
    if (!shot.weapon || token.address != reinterpret_cast<std::uintptr_t>(shot.weapon)
        || !shot.shell_list || !shot.owner || !shot.context
        || !calls.record_at || !calls.create_wrapped)
        return {StormboltShellStatus::invalid_binding};
    StormboltStageState state{};
    if (!store.snapshot(token, state)) return {StormboltShellStatus::stale_instance};
    const auto selector = select_stormbolt_resource(state);
    // Calls deliberately occur after snapshot has released the sidecar lock.
    const void* record = calls.record_at(shot.shell_list, selector);
    if (!record) return {StormboltShellStatus::missing_record, nullptr, selector};
    void* descriptor = nullptr;
    std::memcpy(&descriptor, static_cast<const unsigned char*>(record) + 8,
                sizeof(descriptor));
    if (!descriptor) return {StormboltShellStatus::missing_descriptor, nullptr, selector};
    void* shell = calls.create_wrapped(descriptor, shot.weapon, shot.owner, shot.context);
    return {shell ? StormboltShellStatus::created : StormboltShellStatus::creation_failed,
            shell, selector};
}
} // namespace stormslinger
