#pragma once
#include <cstdint>
#include <cstring>
#include <array>

namespace stormslinger {
struct StormboltPowerCellParameters {
    std::uint32_t threshold;
    std::uint32_t related_count;
    float duration;
    std::array<float,3> multipliers{};
};
// Borrowed, authenticated rPlayerSkillParam object, not cPlayerSkillParam.
// Caller establishes readable extent >= A98, resource lifetime, and serialized
// access across this read. No version/type/lifetime guarantee is supplied here.
// Preserve loaded values, including unusual bit patterns; do not substitute
// constructor defaults or clamp values that native code itself consumes.
inline bool read_stormbolt_power_cell_parameters(
    const void* resource, StormboltPowerCellParameters& output) noexcept {
    if (!resource) return false;
    StormboltPowerCellParameters value{};
    const auto* bytes=static_cast<const unsigned char*>(resource);
    std::memcpy(value.multipliers.data(),bytes+0xA80,12);
    std::memcpy(&value.threshold,bytes+0xA8C,4);
    std::memcpy(&value.related_count,bytes+0xA90,4);
    std::memcpy(&value.duration,bytes+0xA94,4);
    output=value;
    return true;
}
} // namespace stormslinger
