#pragma once
#include "stormbolt_power_cell_loop_state.h"
#include "stormbolt_power_cell_query.h"
#include "stormbolt_shell_hook.h"

namespace stormslinger {
// Native1922190 reads D1 via12F80F0, table5012048/462700 and signed +24.
// Caller supplies a synchronous borrowed owner and authenticated native query.
inline bool read_stormbolt_loop_skill_gate(
    void* owner,std::uintptr_t skillVtable,StormboltSkillValueQuery query,
    StormboltPowerCellLoopUpdateInputs& output) {
    const auto address=reinterpret_cast<std::uintptr_t>(owner);
    std::uint8_t flags=0;
    if (!read_stormbolt_field(address,0x60,flags)) return false;
    auto candidate=output;
    candidate.owner_flag=(flags&1)!=0;
    candidate.resource_index_present=false;
    candidate.resource_count=0;
    if (candidate.owner_flag) {
        std::uintptr_t component=0,vtable=0;
        std::int32_t index=0;
        if (!skillVtable || !query ||
            !read_stormbolt_field(address,0x126D8,component) || !component ||
            !read_stormbolt_field(component,0,vtable) || vtable!=skillVtable ||
            !read_stormbolt_field(component,0x141C,index)) return false;
        candidate.resource_index_present=index!=0;
        if (index) candidate.resource_count=static_cast<std::int16_t>(query(owner,0xD1,0,0));
    }
    output=candidate;
    return true;
}
} // namespace stormslinger
