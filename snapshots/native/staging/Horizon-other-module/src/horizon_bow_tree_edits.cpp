#include "horizon_bow_tree_edits.h"

namespace horizon_other {
namespace {
struct TreeEdit {
    std::uint32_t owner;
    std::size_t offset;
    std::size_t count;
    std::uint8_t expected[6];
};
#include "generated_bow_tree_edits.inc"
}

const TreeOwnerSpec* bow_tree_owner(std::uint32_t owner_rva) noexcept {
    for (const auto& owner : kTreeOwners)
        if (owner.rva == owner_rva) return &owner;
    return nullptr;
}

TreeEditResult compose_bow_tree_edits(std::uint32_t owner_rva, std::uint8_t* clone,
                                     std::size_t bytes) noexcept {
    const auto* owner = bow_tree_owner(owner_rva);
    if (!owner) return TreeEditResult::unsupported_owner;
    if (!clone || bytes != owner->bytes) return TreeEditResult::invalid_buffer;
    bool changed = false;
    // Validate ALL sites before the first write. A conflict leaves the clone
    // untouched; NOPs from an earlier application are accepted as a whole site.
    for (const auto& edit : kTreeEdits) {
        if (edit.owner != owner_rva) continue;
        if (edit.offset > bytes || edit.count > bytes - edit.offset)
            return TreeEditResult::invalid_buffer;
        bool original = true, applied = true;
        for (std::size_t i = 0; i < edit.count; ++i) {
            original = original && clone[edit.offset + i] == edit.expected[i];
            applied = applied && clone[edit.offset + i] == 0x90;
        }
        if (!original && !applied) return TreeEditResult::conflict;
        changed = changed || !applied;
    }
    if (!changed) return TreeEditResult::unchanged;
    for (const auto& edit : kTreeEdits) {
        if (edit.owner != owner_rva) continue;
        for (std::size_t i = 0; i < edit.count; ++i) clone[edit.offset + i] = 0x90;
    }
    return TreeEditResult::applied;
}
}
