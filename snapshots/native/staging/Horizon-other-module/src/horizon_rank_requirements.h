#pragma once
#include <cstdint>

namespace horizon_other {
enum class RankKind { none, hunter, master };
struct RankRequirement {
    std::uint16_t code;
    RankKind kind;
    int minimum;
};
enum class RankFloorResult { unrelated_quest, unknown_condition, invalid_rank, unmet, met };
const RankRequirement* source_rank_requirement(std::uint16_t code) noexcept;
// Checks only the source rank floor. A result of met must NEVER be used as the
// complete quest-open/admission decision: other native prerequisites remain.
RankFloorResult source_quest_rank_floor(std::int32_t quest_id, int hunter_rank,
                                       int master_rank) noexcept;
enum class QuestClearFact { unknown, not_cleared, cleared };
enum class QuestRequirementResult { unrelated_quest, invalid_rank, unknown_condition,
                                    rank_unmet, clear_unknown, prerequisite_unmet, requirements_met,
                                    native_state_unavailable };
// Known source prerequisite only. Zero does not certify absence of every
// additional native admission rule; retain original session/story checks.
std::int32_t source_required_quest_clear(std::int32_t quest_id) noexcept;
QuestRequirementResult source_quest_open_requirements(std::int32_t quest_id,
    int hunter_rank, int master_rank, QuestClearFact required_clear) noexcept;
}
