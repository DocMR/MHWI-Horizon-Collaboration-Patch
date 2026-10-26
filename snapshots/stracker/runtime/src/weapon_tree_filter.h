#pragma once
#include <cstdint>
namespace stormslinger {
// Native equipment IDs, not row indexes. Applies only to the tree exclusion.
// This helper must never replace recipe availability or crafting cost checks.
inline bool isStormslingerTreeEntry(unsigned category, unsigned equipmentId) noexcept {
 return category == 13 && (equipmentId == 238 || equipmentId == 239);
}
}
