#pragma once
#include "horizon_rank_requirements.h"
#include "horizon_quest_mapping.h"
namespace horizon_other {
// Caller owns the character lifetime and serializes mapping changes/reset/load.
// This reads only this module's explicit prerequisite; no completion is granted.
QuestClearFact read_source_required_clear(const void* native_character, std::int32_t quest_id,
                                         const PreparedQuestMapping& mapping) noexcept;
enum class QuestOpenFact { unknown, closed, open };
QuestOpenFact read_source_quest_open(const void* native_character, std::int32_t quest_id,
                                    const PreparedQuestMapping& mapping) noexcept;
}
