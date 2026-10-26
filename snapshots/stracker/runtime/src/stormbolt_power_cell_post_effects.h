#pragma once
#include "stormbolt_shell_hook.h"
#include "stormbolt_self_damage_effect.h"
#include <array>

namespace stormslinger {
struct StormboltPowerCellPostEffectsResult {
    std::array<bool,2> invoked{};
    std::array<void*,2> native_results{};
};
// PS2CEE659..68D: two body requests, preserving order and reloading primary
// between calls. The caller selects continue_effects independently of count.
inline bool request_stormbolt_power_cell_post_effects(void* owner,
    StormboltEquipmentEffectRequest request,StormboltPowerCellPostEffectsResult& output) {
    if (!owner || !request) return false;
    output={};
    for (std::uint32_t record=0;record<2;++record) {
        void* primary=nullptr;
        if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(owner),0x13D58,primary) || !primary) return false;
        output.native_results[record]=request(primary,1,0x7D0,record,true);
        output.invoked[record]=true;
    }
    return true;
}
} // namespace stormslinger
