#pragma once
#include <cstddef>
#include <cstdint>

namespace horizon_other {
struct ArmorRecipeIdentity {
    std::uint8_t category;
    std::uint16_t serialized_row;
    std::uint16_t native_equipment_id;
};
const ArmorRecipeIdentity* armor_recipe_identities(std::size_t* count) noexcept;
// ONLY postprocess the result of certified native predicate278A00, after the
// original call. This is not a crafting permission or material/story bypass.
bool horizon_armor_exclusion_result(const void* native_recipe, bool original_excluded) noexcept;
}
