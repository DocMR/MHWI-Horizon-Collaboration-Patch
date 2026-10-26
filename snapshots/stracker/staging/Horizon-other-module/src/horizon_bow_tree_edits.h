#pragma once
#include <cstddef>
#include <cstdint>

namespace horizon_other {
struct TreeOwnerSpec {
    std::uint32_t rva;
    std::size_t bytes;
    std::uint8_t original_sha256[32];
};
enum class TreeEditResult { unsupported_owner, invalid_buffer, conflict, unchanged, applied };
const TreeOwnerSpec* bow_tree_owner(std::uint32_t owner_rva) noexcept;
// Shared bootstrap must certify the original image/owner digest BEFORE cloning.
// Apply to a writable, unrelocated clone buffer before final relocation. Other
// modules may edit nonoverlapping bytes in this same buffer. This installs no
// hooks, allocates no executable memory, and never edits the original image.
TreeEditResult compose_bow_tree_edits(std::uint32_t owner_rva, std::uint8_t* clone,
                                     std::size_t bytes) noexcept;
}
