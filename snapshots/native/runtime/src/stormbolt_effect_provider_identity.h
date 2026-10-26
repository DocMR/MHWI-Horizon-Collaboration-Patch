#pragma once
#include "stormbolt_native_memory.h"
#include "stormbolt_player_effect_adapter.h"

namespace stormslinger {
struct StormboltEffectProviderIdentity {
    void** singleton_slot = nullptr;
    std::uintptr_t vtable = 0, dti = 0, dti_vtable = 0;
    std::uintptr_t dti_getter = 0, record_create = 0;
    StormboltEffectCreate create = nullptr;
};
enum class StormboltProviderStatus {
    matched, invalid_identity, unreadable, absent, wrong_type, changed_dispatch, changed_during_read
};
struct StormboltProviderMemory {
    template<class T> static bool read(std::uintptr_t base, std::size_t offset, T& output) noexcept {
        return read_stormbolt_field(base,offset,output);
    }
};

// Read-only identity snapshot. No native function is called. Bootstrap must
// authenticate these expected addresses/code first. Caller must separately
// establish singleton/DTI lifetime and synchronization across this read and its
// eventual use: repeated reads detect some changes, not ABA or races. A matched
// result proves neither resource readiness nor the full action preflight.
// Consume provider+create together; do not re-read a different singleton and
// call a cached implementation on it. Failure leaves output unchanged.
template<class Memory = StormboltProviderMemory>
StormboltProviderStatus snapshot_stormbolt_effect_provider(
    const StormboltEffectProviderIdentity& expected, StormboltEffectProviderSnapshot& output) noexcept {
    if (!expected.singleton_slot || !expected.vtable || !expected.dti || !expected.dti_vtable ||
        !expected.dti_getter || !expected.record_create || !expected.create)
        return StormboltProviderStatus::invalid_identity;
    std::uintptr_t provider = 0, table = 0, dti_table = 0, getter = 0, create = 0, record = 0;
    const auto slot = reinterpret_cast<std::uintptr_t>(expected.singleton_slot);
    if (!Memory::read(slot,0,provider)) return StormboltProviderStatus::unreadable;
    if (!provider) return StormboltProviderStatus::absent;
    if (!Memory::read(provider,0,table) || !Memory::read(expected.dti,0,dti_table))
        return StormboltProviderStatus::unreadable;
    if (table != expected.vtable || dti_table != expected.dti_vtable)
        return StormboltProviderStatus::wrong_type;
    if (!Memory::read(table,0x20,getter) || !Memory::read(table,0x108,create) ||
        !Memory::read(table,0xF8,record)) return StormboltProviderStatus::unreadable;
    if (getter != expected.dti_getter || create != reinterpret_cast<std::uintptr_t>(expected.create) ||
        record != expected.record_create) return StormboltProviderStatus::changed_dispatch;
    std::uintptr_t current = 0, current_table = 0, current_dti_table = 0;
    if (!Memory::read(slot,0,current) || !Memory::read(provider,0,current_table) ||
        !Memory::read(expected.dti,0,current_dti_table)) return StormboltProviderStatus::unreadable;
    if (current != provider || current_table != table || current_dti_table != dti_table)
        return StormboltProviderStatus::changed_during_read;
    output = {reinterpret_cast<void*>(provider),reinterpret_cast<StormboltEffectCreate>(create)};
    return StormboltProviderStatus::matched;
}
inline bool resolve_stormbolt_effect_provider(const void* context, StormboltEffectProviderSnapshot& output) noexcept {
    return context && snapshot_stormbolt_effect_provider(
        *static_cast<const StormboltEffectProviderIdentity*>(context),output) == StormboltProviderStatus::matched;
}
} // namespace stormslinger
