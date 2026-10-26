#pragma once
#include "stormbolt_shell_hook.h"
#include <array>

namespace stormslinger {
struct StormboltPowerCellRemoteCache {
    std::uintptr_t actor=0;
    std::array<std::uint32_t,5> current{},previous{};
};
// PC11B3FB0 fills current; PC11B034A..0380 overwrites previous.
// Capture on the game thread before the native history-copy tail. This reads
// raw fields only and deliberately makes no unverified wire-bit assignment.
inline bool read_stormbolt_power_cell_remote_cache(
    void* component,StormboltPowerCellRemoteCache& output) noexcept {
    if (!component) return false;
    const auto base=reinterpret_cast<std::uintptr_t>(component);
    StormboltPowerCellRemoteCache value{};
    if (!read_stormbolt_field(base,0xDB0,value.actor) || !value.actor) return false;
    for (std::size_t i=0;i<value.current.size();++i) {
        if (!read_stormbolt_field(base,0x5998+i*4,value.current[i]) ||
            !read_stormbolt_field(base,0x59B0+i*4,value.previous[i])) return false;
    }
    output=value;
    return true;
}
} // namespace stormslinger
