#include "horizon_palico_recipe_edits.h"

namespace horizon_other {
namespace {
struct PalicoEdit {
    std::size_t offset, count;
    std::uint8_t before[6], after[6];
};
#include "generated_palico_recipe_edits.inc"
}
const TreeOwnerSpec& palico_recipe_owner() noexcept { return kPalicoOwner; }
TreeEditResult compose_palico_recipe_edits(std::uint32_t owner_rva,
    std::uint8_t* clone, std::size_t bytes) noexcept {
    if (owner_rva != kPalicoOwner.rva) return TreeEditResult::unsupported_owner;
    if (!clone || bytes != kPalicoOwner.bytes) return TreeEditResult::invalid_buffer;
    bool changed = false;
    for (const auto& edit : kPalicoEdits) {
        if (edit.offset > bytes || edit.count > bytes - edit.offset)
            return TreeEditResult::invalid_buffer;
        bool before = true, after = true;
        for (std::size_t i = 0; i < edit.count; ++i) {
            before = before && clone[edit.offset + i] == edit.before[i];
            after = after && clone[edit.offset + i] == edit.after[i];
        }
        if (!before && !after) return TreeEditResult::conflict;
        changed = changed || !after;
    }
    if (!changed) return TreeEditResult::unchanged;
    for (const auto& edit : kPalicoEdits)
        for (std::size_t i = 0; i < edit.count; ++i) clone[edit.offset + i] = edit.after[i];
    return TreeEditResult::applied;
}
}
