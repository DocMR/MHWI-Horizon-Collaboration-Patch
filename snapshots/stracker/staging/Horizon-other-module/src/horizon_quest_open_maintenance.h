#pragma once
#include "horizon_native_rank_reader.h"
namespace horizon_other {
enum class QuestOpenRefreshStatus { unchanged, updated, unavailable };
struct QuestOpenRefreshResult { QuestOpenRefreshStatus status; std::size_t opened; };
class NativeQuestOpenMaintenance {
    using OpenWriter = void (*)(void*, std::int32_t);
    OpenWriter open_ = nullptr;
    const std::uint8_t* image_ = nullptr;
    NativeQuestRankReader ranks_;
public:
    // Bind BEFORE mapping redirection changes the native writer's code bytes.
    bool bind(const std::uint8_t* certified_image) noexcept;
    QuestRequirementResult known_requirements(const void* native_character, std::int32_t quest_id,
                                               const PreparedQuestMapping& mapping) const noexcept;
    // Caller owns character lifetime and serializes load/reset/rank/clear/map
    // transitions. Call after original native state updates, never during reset.
    // Only grants source-required open bits; never completion, rewards or SOS.
    QuestOpenRefreshResult refresh(void* native_character, const PreparedQuestMapping& mapping) const noexcept;
};
}
