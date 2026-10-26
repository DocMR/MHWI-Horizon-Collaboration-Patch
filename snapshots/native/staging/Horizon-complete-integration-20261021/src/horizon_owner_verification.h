#pragma once
#include "horizon_bow_tree_edits.h"
namespace horizon_other {
// Hash the live original owner, not its encrypted on-disk representation.
// Full executable/build certification remains the bootstrap's responsibility.
bool verify_native_owner(const std::uint8_t* image, const TreeOwnerSpec& owner) noexcept;
}
