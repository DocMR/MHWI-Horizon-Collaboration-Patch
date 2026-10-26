#pragma once
#include <cstddef>
#include <cstdint>

namespace horizon_other {
// Metadata only. Finding a quest does not grant any native open/clear state.
struct QuestDescriptor {
    std::int32_t id;
    std::uint8_t star;
    std::uint32_t source_rank;
    std::uint32_t reward_id;
    const char* resource_name;
    std::uint16_t source_condition_a;
    std::uint16_t source_condition_b;
};
const QuestDescriptor* quest_catalog(std::size_t& count) noexcept;
const QuestDescriptor* find_quest(std::int32_t id) noexcept;
}
