#pragma once
#include <array>
#include <cstdint>
// Analysis candidate only. A runtime transaction must verify and restore bytes.
// Only explicit LBG ID238/239 exclusions are removed; native recipe gates remain.
namespace stormslinger {
struct WeaponTreeBranchSpec { std::uint32_t rva; unsigned size; std::array<unsigned char,6> expected; };
inline constexpr WeaponTreeBranchSpec weaponTreeBranches[] = {
 {0x137348Eu, 6u, {0x0F,0x86,0x46,0x01,0x00,0x00}},
 {0x1373917u, 2u, {0x76,0x08}},
};
}
