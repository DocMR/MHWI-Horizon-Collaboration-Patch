#include "horizon_native_rank_reader.h"
#include "horizon_owner_verification.h"
#include "horizon_quest_catalog.h"
#include <limits>
namespace horizon_other {
namespace {
#include "generated_native_rank_owners.inc"
}
bool NativeQuestRankReader::bind(const std::uint8_t* image) noexcept {
    if (hunter_ || master_ || !image) return false;
    for (const auto& owner : kNativeRankOwners)
        if (!verify_native_owner(image, owner)) return false;
    hunter_ = reinterpret_cast<Reader>(const_cast<std::uint8_t*>(image)+0x1354AF0);
    master_ = reinterpret_cast<Reader>(const_cast<std::uint8_t*>(image)+0x13540B0);
    return true;
}
QuestRequirementResult NativeQuestRankReader::read_requirements(const void* character,
    std::int32_t id, const PreparedQuestMapping& mapping) const noexcept {
    if (!find_quest(id)) return QuestRequirementResult::unrelated_quest;
    if (!character || !hunter_ || !master_) return QuestRequirementResult::native_state_unavailable;
    const auto hunter = hunter_(character), master = master_(character);
    if (hunter > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()) ||
        master > static_cast<std::uint32_t>((std::numeric_limits<int>::max)()))
        return QuestRequirementResult::invalid_rank;
    return source_quest_open_requirements(id, static_cast<int>(hunter), static_cast<int>(master),
                                          read_source_required_clear(character, id, mapping));
}
}
