#pragma once
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltPowerCellState {
    std::uint32_t count=0;
    float timer=-1.0f;
    bool event_eligible=false; // PS status+A14; sidecar, never a PC+A14 write.
};
// Distinct token type prevents accidental use of a weapon-generation token.
struct StormboltPowerCellToken {
    std::uintptr_t address=0;
    std::uint64_t generation=0;
    explicit operator bool() const noexcept { return address && generation; }
};
// Key is the native status-manager identity, authenticated by lifecycle hooks.
// Construction/reset/retirement must follow native status-manager lifecycle;
// weapon switching does not initialize or retire this store. Borrowed native
// pointers remain subject to external lifetime and game-thread guarantees.
template<std::size_t Capacity>
using StormboltPowerCellStore=StormboltInstanceStore<Capacity,
    StormboltPowerCellState,StormboltPowerCellToken>;
} // namespace stormslinger
