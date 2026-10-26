#pragma once
#include "stormbolt_power_cell_remote_cache.h"
#include "stormbolt_power_cell_remote_decision.h"

namespace stormslinger {
// Patched PC peers use the final packet DWORD30 / current[4]. The native
// native bit31 is proven zero even for arbitrary raw status bytes. Music
// remains DWORD24. Native apply consumes low24; payload24..27 replaces
// ignored spill bits while preserving low24 and native bits28..30.
inline constexpr std::uint32_t stormbolt_power_cell_sync_marker=0x80000000u;
inline constexpr std::uint32_t stormbolt_power_cell_sync_mask=0x8F000000u;
inline bool encode_stormbolt_power_cell_sync(std::uint32_t native_word,
    StormboltPowerCellRemoteFlags flags,std::uint32_t& output) noexcept {
    // The input is freshly generated native data, never our previous packet.
    // A native word never has the marker; reject accidental re-encoding.
    if (native_word&stormbolt_power_cell_sync_marker) return false;
    output=(native_word&~stormbolt_power_cell_sync_mask) | stormbolt_power_cell_sync_marker | (flags.empty ? 0x01000000u : 0u)
        | (flags.below ? 0x02000000u : 0u)
        | (flags.reached ? 0x04000000u : 0u)
        | (flags.effect ? 0x08000000u : 0u);
    return true;
}
inline bool decode_stormbolt_power_cell_sync(const StormboltPowerCellRemoteCache& cache,
    StormboltPowerCellRemoteFlags& flags,bool& previous_effect) noexcept {
    const auto word=cache.current[4];
    if (!(word&stormbolt_power_cell_sync_marker)) return false;
    flags={bool(word&0x01000000u),bool(word&0x02000000u),
        bool(word&0x04000000u),bool(word&0x08000000u)};
    previous_effect=(cache.previous[4]&stormbolt_power_cell_sync_marker)!=0 &&
        (cache.previous[4]&0x08000000u)!=0;
    return true;
}
} // namespace stormslinger
