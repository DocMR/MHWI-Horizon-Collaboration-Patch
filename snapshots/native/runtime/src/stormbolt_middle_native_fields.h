#pragma once
#include <cstdint>
#include <cstring>

namespace stormslinger {
// Authenticated PC layout: entry condition PS1180 -> PC1128; replicated
// stage PS13EDE -> PC13E7E. Caller owns the live actor and excludes concurrent
// destruction; these accessors do not establish lifetime or synchronization.
inline bool stormbolt_middle_needs_parameter(void* actor) noexcept {
    std::uint32_t value{};
    std::memcpy(&value, static_cast<const unsigned char*>(actor)+0x1128, sizeof(value));
    return value == 0;
}
inline std::uint8_t stormbolt_middle_read_stage(void* actor) noexcept {
    std::uint8_t value{};
    std::memcpy(&value, static_cast<const unsigned char*>(actor)+0x13E7E, sizeof(value));
    return value;
}
inline void stormbolt_middle_write_stage(void* actor, std::uint8_t value) noexcept {
    std::memcpy(static_cast<unsigned char*>(actor)+0x13E7E, &value, sizeof(value));
}
} // namespace stormslinger
