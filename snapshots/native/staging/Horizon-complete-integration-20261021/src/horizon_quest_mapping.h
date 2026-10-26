#pragma once
#include <cstddef>
#include <cstdint>

namespace horizon_other {
struct QuestMappingCandidate {
    const std::int32_t* entries;
    std::size_t count; // Excludes the terminal -1.
    std::size_t original_count;
};
// A readonly original-prefix-preserving candidate, NOT permission to allocate
// its proposed save indices. No memory redirection or save writes occur here.
QuestMappingCandidate quest_mapping_candidate() noexcept;
struct QuestMappingRedirect {
    std::uint32_t displacement_rva;
    std::uint8_t before[4], after[4];
};
enum class MappingRedirectStatus { prepared, invalid_buffer, original_mismatch, candidate_mismatch, out_of_range,
                                   already_prepared, allocation_failed, protection_failed };
struct MappingRedirectResult { MappingRedirectStatus status; std::size_t count; };
std::size_t quest_mapping_reference_count() noexcept;
// Prepare four-byte displacement edits only. Does not install them or allocate
// save indices. Host must supply an already allocated exact candidate table,
// certify the executable, and resolve lifecycle/ownership before publication.
MappingRedirectResult prepare_quest_mapping_redirects(const std::uint8_t* certified_image,
    const std::int32_t* candidate_table, QuestMappingRedirect* output, std::size_t capacity) noexcept;
// Successful candidate allocations remain valid for the process lifetime.
// Preparation does not publish any instruction edit or grant save-index ownership.
class PreparedQuestMapping {
    const std::int32_t* table_ = nullptr;
    const std::uint8_t* image_ = nullptr;
    QuestMappingRedirect redirects_[64]{};
    std::size_t count_ = 0;
public:
    PreparedQuestMapping() = default;
    PreparedQuestMapping(const PreparedQuestMapping&) = delete;
    PreparedQuestMapping& operator=(const PreparedQuestMapping&) = delete;
    MappingRedirectResult prepare(const std::uint8_t* certified_image) noexcept;
    const std::int32_t* table() const noexcept { return table_; }
    const std::uint8_t* image() const noexcept { return image_; }
    const QuestMappingRedirect* redirects() const noexcept { return redirects_; }
    std::size_t redirect_count() const noexcept { return count_; }
};
// Read-only check that every known reference currently points at this candidate.
bool quest_mapping_references_active(const PreparedQuestMapping& prepared) noexcept;
}
