#pragma once
#include "horizon_quest_completion_fact.h"
namespace horizon_other {
class NativeQuestRankReader {
    using Reader = std::uint32_t (*)(const void*);
    Reader hunter_ = nullptr, master_ = nullptr;
public:
    // Host first certifies the whole executable. Binding verifies exact reader
    // bytes, including the native HR mode dependency; this installs no Hook.
    bool bind(const std::uint8_t* certified_image) noexcept;
    // Caller owns the character lifetime and serializes reset/load/map changes.
    // requirements_met still does not replace other native admission checks.
    QuestRequirementResult read_requirements(const void* native_character, std::int32_t quest_id,
                                             const PreparedQuestMapping& mapping) const noexcept;
};
}
