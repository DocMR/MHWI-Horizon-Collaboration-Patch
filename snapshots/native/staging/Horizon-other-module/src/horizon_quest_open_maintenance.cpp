#include "horizon_quest_open_maintenance.h"
#include "horizon_owner_verification.h"
#include "horizon_quest_catalog.h"
namespace horizon_other {
namespace {
#include "generated_quest_open_owner.inc"
}
bool NativeQuestOpenMaintenance::bind(const std::uint8_t* image) noexcept {
    if (!image || open_ || !verify_native_owner(image, kNativeOpenOwner) || !ranks_.bind(image)) return false;
    image_ = image;
    open_ = reinterpret_cast<OpenWriter>(const_cast<std::uint8_t*>(image)+0x13697A0);
    return true;
}
QuestRequirementResult NativeQuestOpenMaintenance::known_requirements(const void* character,
    std::int32_t id, const PreparedQuestMapping& mapping) const noexcept {
    if (!open_ || !character || mapping.image() != image_ || !quest_mapping_references_active(mapping))
        return QuestRequirementResult::native_state_unavailable;
    return ranks_.read_requirements(character, id, mapping);
}
QuestOpenRefreshResult NativeQuestOpenMaintenance::refresh(void* character,
    const PreparedQuestMapping& mapping) const noexcept {
    if (!open_ || !character || mapping.image() != image_ || !quest_mapping_references_active(mapping))
        return {QuestOpenRefreshStatus::unavailable, 0};
    std::size_t count = 0;
    const auto* quests = quest_catalog(count);
    std::int32_t pending[6]{};
    std::size_t needed = 0;
    if (count > sizeof(pending)/sizeof(pending[0])) return {QuestOpenRefreshStatus::unavailable, 0};
    // Read all facts before publishing any open-state change.
    for (std::size_t n = 0; n < count; ++n) {
        const auto requirements = ranks_.read_requirements(character, quests[n].id, mapping);
        if (requirements == QuestRequirementResult::rank_unmet ||
            requirements == QuestRequirementResult::prerequisite_unmet) continue;
        if (requirements != QuestRequirementResult::requirements_met) return {QuestOpenRefreshStatus::unavailable, 0};
        const auto state = read_source_quest_open(character, quests[n].id, mapping);
        if (state == QuestOpenFact::unknown) return {QuestOpenRefreshStatus::unavailable, 0};
        if (state == QuestOpenFact::closed) pending[needed++] = quests[n].id;
    }
    for (std::size_t n = 0; n < needed; ++n) open_(character, pending[n]);
    return {needed ? QuestOpenRefreshStatus::updated : QuestOpenRefreshStatus::unchanged, needed};
}
}
