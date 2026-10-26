#include "horizon_rank_requirements.h"
#include "horizon_quest_catalog.h"

namespace horizon_other {
namespace {
constexpr RankRequirement requirements[] = {
#include "generated_rank_conditions.inc"
};
bool rank_met(const RankRequirement& requirement, int hunter_rank, int master_rank) noexcept {
    if (requirement.kind == RankKind::hunter) return hunter_rank >= requirement.minimum;
    if (requirement.kind == RankKind::master) return master_rank >= requirement.minimum;
    return true;
}
}

const RankRequirement* source_rank_requirement(std::uint16_t code) noexcept {
    for (const auto& requirement : requirements) {
        if (requirement.code == code) return &requirement;
    }
    return nullptr;
}

RankFloorResult source_quest_rank_floor(std::int32_t quest_id, int hunter_rank,
                                       int master_rank) noexcept {
    const auto quest = find_quest(quest_id);
    if (!quest) return RankFloorResult::unrelated_quest;
    if (hunter_rank < 0 || master_rank < 0) return RankFloorResult::invalid_rank;
    const auto first = source_rank_requirement(quest->source_condition_a);
    const auto second = source_rank_requirement(quest->source_condition_b);
    if (!first || !second) return RankFloorResult::unknown_condition;
    return rank_met(*first, hunter_rank, master_rank) && rank_met(*second, hunter_rank, master_rank)
        ? RankFloorResult::met : RankFloorResult::unmet;
}
std::int32_t source_required_quest_clear(std::int32_t quest_id) noexcept {
    // Capcom's official event schedule: Firebreak follows The Survivor.
    return quest_id == 67804 ? 67803 : 0;
}
QuestRequirementResult source_quest_open_requirements(std::int32_t quest_id,
    int hunter_rank, int master_rank, QuestClearFact clear) noexcept {
    switch (source_quest_rank_floor(quest_id, hunter_rank, master_rank)) {
    case RankFloorResult::unrelated_quest: return QuestRequirementResult::unrelated_quest;
    case RankFloorResult::unknown_condition: return QuestRequirementResult::unknown_condition;
    case RankFloorResult::invalid_rank: return QuestRequirementResult::invalid_rank;
    case RankFloorResult::unmet: return QuestRequirementResult::rank_unmet;
    case RankFloorResult::met: break;
    }
    if (source_required_quest_clear(quest_id)) {
        if (clear == QuestClearFact::unknown) return QuestRequirementResult::clear_unknown;
        if (clear != QuestClearFact::cleared) return QuestRequirementResult::prerequisite_unmet;
    }
    return QuestRequirementResult::requirements_met;
}
}
