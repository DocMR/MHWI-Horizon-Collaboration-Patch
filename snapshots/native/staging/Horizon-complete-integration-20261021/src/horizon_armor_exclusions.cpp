#include "horizon_armor_exclusions.h"
#include <cstring>

namespace horizon_other {
namespace {
constexpr ArmorRecipeIdentity kArmorRecipes[] = {
#include "generated_armor_recipe_exclusions.inc"
};
}
const ArmorRecipeIdentity* armor_recipe_identities(std::size_t* count) noexcept {
    if (count) *count = sizeof(kArmorRecipes) / sizeof(kArmorRecipes[0]);
    return kArmorRecipes;
}
bool horizon_armor_exclusion_result(const void* native_recipe, bool original_excluded) noexcept {
    if (!original_excluded || !native_recipe) return original_excluded;
    const auto* bytes = static_cast<const std::uint8_t*>(native_recipe);
    const std::uint8_t category = bytes[8];
    if (category > 4) return original_excluded;
    std::uint16_t row = 0;
    std::memcpy(&row, bytes + 10, sizeof(row));
    for (const auto& target : kArmorRecipes)
        if (target.category == category && target.serialized_row == row) return false;
    return original_excluded;
}
}
