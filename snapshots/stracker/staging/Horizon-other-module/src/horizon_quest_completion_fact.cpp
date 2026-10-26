#include "horizon_quest_completion_fact.h"
#include "horizon_quest_catalog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace horizon_other {
namespace {
bool read_source_bit(const void* character, std::int32_t id, const PreparedQuestMapping& mapping,
                     std::size_t field, bool* set) noexcept {
    if (!find_quest(id) || !character || !quest_mapping_references_active(mapping)) return false;
    const auto candidate = quest_mapping_candidate();
    std::size_t index = candidate.count;
    for (std::size_t n = 0; n < candidate.count; ++n)
        if (candidate.entries[n] == id) { index = n; break; }
    if (index >= candidate.count || index >= 512) return false;
    std::uint32_t word = 0;
    SIZE_T read = 0;
    const auto* address = static_cast<const std::uint8_t*>(character)+field+(index/32)*4;
    if (!ReadProcessMemory(GetCurrentProcess(), address, &word, sizeof(word), &read) || read != sizeof(word))
        return false;
    *set = (word & (std::uint32_t(1) << (index%32))) != 0;
    return true;
}
}
QuestClearFact read_source_required_clear(const void* character, std::int32_t id,
                                         const PreparedQuestMapping& mapping) noexcept {
    const auto required = source_required_quest_clear(id);
    bool set = false;
    if (!required || !read_source_bit(character, required, mapping, 0x104C20, &set)) return QuestClearFact::unknown;
    return set ? QuestClearFact::cleared : QuestClearFact::not_cleared;
}
QuestOpenFact read_source_quest_open(const void* character, std::int32_t id,
                                    const PreparedQuestMapping& mapping) noexcept {
    bool set = false;
    if (!read_source_bit(character, id, mapping, 0x104BD8, &set)) return QuestOpenFact::unknown;
    return set ? QuestOpenFact::open : QuestOpenFact::closed;
}
}
