#pragma once
#include "horizon_bow_tree_edits.h"

namespace horizon_other {
const TreeOwnerSpec& palico_recipe_owner() noexcept;
// Shared bootstrap must certify original owner digest, clone its complete646
// byte span, compose these edits, then relocate/install one hook. No executable
// memory or original game image is touched by this component.
TreeEditResult compose_palico_recipe_edits(std::uint32_t owner_rva,
    std::uint8_t* unrelocated_clone, std::size_t bytes) noexcept;
}
