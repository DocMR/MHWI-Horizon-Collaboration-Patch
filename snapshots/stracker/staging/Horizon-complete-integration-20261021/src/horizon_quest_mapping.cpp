#include "horizon_quest_mapping.h"
#include "generated_quest_save_mapping.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <limits>
#include <initializer_list>

namespace horizon_other {
namespace {
struct MappingReference {
    std::uint32_t rva;
    std::uint8_t size, displacement_offset;
    std::uint32_t table_index;
    std::uint8_t expected[7];
};
#include "generated_quest_mapping_references.inc"
constexpr std::size_t reference_count = sizeof(kMappingReferences)/sizeof(kMappingReferences[0]);
bool read_exact(const void* source, void* output, std::size_t bytes) noexcept {
    SIZE_T read = 0;
    return ReadProcessMemory(GetCurrentProcess(), source, output, bytes, &read) && read == bytes;
}
}
QuestMappingCandidate quest_mapping_candidate() noexcept {
    return {generated::quest_mapping_candidate,
            sizeof(generated::quest_mapping_candidate) /
                sizeof(generated::quest_mapping_candidate[0]) - 1,
            generated::native_quest_count};
}
std::size_t quest_mapping_reference_count() noexcept { return reference_count; }
MappingRedirectResult prepare_quest_mapping_redirects(const std::uint8_t* image,
    const std::int32_t* table, QuestMappingRedirect* output, std::size_t capacity) noexcept {
    if (!image || !table || !output || capacity < reference_count)
        return {MappingRedirectStatus::invalid_buffer, 0};
    std::int32_t copied[sizeof(generated::quest_mapping_candidate)/sizeof(std::int32_t)]{};
    if (!read_exact(table, copied, sizeof(copied)) ||
        std::memcmp(copied, generated::quest_mapping_candidate, sizeof(copied)))
        return {MappingRedirectStatus::candidate_mismatch, 0};
    const auto original_bytes = (generated::native_quest_count+1)*sizeof(std::int32_t);
    if (!read_exact(image+0x324FAC0, copied, original_bytes) ||
        std::memcmp(copied, generated::quest_mapping_candidate, generated::native_quest_count*sizeof(std::int32_t)) ||
        copied[generated::native_quest_count] != -1)
        return {MappingRedirectStatus::original_mismatch, 0};
    QuestMappingRedirect pending[reference_count]{};
    for (std::size_t n = 0; n < reference_count; ++n) {
        const auto& ref = kMappingReferences[n];
        std::uint8_t instruction[7]{};
        if (!read_exact(image+ref.rva, instruction, ref.size) || std::memcmp(instruction, ref.expected, ref.size))
            return {MappingRedirectStatus::original_mismatch, 0};
        const auto target = reinterpret_cast<std::uintptr_t>(table)+ref.table_index*sizeof(std::int32_t);
        const auto next = reinterpret_cast<std::uintptr_t>(image)+ref.rva+ref.size;
        const auto delta = static_cast<std::int64_t>(target)-static_cast<std::int64_t>(next);
        if (delta < (std::numeric_limits<std::int32_t>::min)() || delta > (std::numeric_limits<std::int32_t>::max)())
            return {MappingRedirectStatus::out_of_range, 0};
        const auto displacement = static_cast<std::int32_t>(delta);
        pending[n].displacement_rva = ref.rva+ref.displacement_offset;
        std::memcpy(pending[n].before, instruction+ref.displacement_offset, 4);
        std::memcpy(pending[n].after, &displacement, 4);
    }
    std::memcpy(output, pending, sizeof(pending));
    return {MappingRedirectStatus::prepared, reference_count};
}
MappingRedirectResult PreparedQuestMapping::prepare(const std::uint8_t* image) noexcept {
    if (table_) return {MappingRedirectStatus::already_prepared, count_};
    if (!image || reference_count > sizeof(redirects_)/sizeof(redirects_[0]))
        return {MappingRedirectStatus::invalid_buffer, 0};
    const auto origin = (reinterpret_cast<std::uintptr_t>(image)+0x324FAC0) & ~std::uintptr_t(0xffff);
    void* allocation = nullptr;
    for (std::uintptr_t distance = 0; distance < 0x4000000 && !allocation; distance += 0x10000) {
        for (int direction : {1, -1}) {
            if (direction < 0 && origin < distance) continue;
            const auto address = direction > 0 ? origin+distance : origin-distance;
            allocation = VirtualAlloc(reinterpret_cast<void*>(address), 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (allocation) break;
        }
    }
    if (!allocation) return {MappingRedirectStatus::allocation_failed, 0};
    static_assert(sizeof(generated::quest_mapping_candidate) <= 4096, "Candidate exceeds allocation");
    std::memcpy(allocation, generated::quest_mapping_candidate, sizeof(generated::quest_mapping_candidate));
    const auto result = prepare_quest_mapping_redirects(image, static_cast<const std::int32_t*>(allocation),
                                                       redirects_, sizeof(redirects_)/sizeof(redirects_[0]));
    if (result.status != MappingRedirectStatus::prepared) {
        VirtualFree(allocation, 0, MEM_RELEASE);
        return result;
    }
    DWORD previous = 0;
    if (!VirtualProtect(allocation, 4096, PAGE_READONLY, &previous)) {
        VirtualFree(allocation, 0, MEM_RELEASE);
        return {MappingRedirectStatus::protection_failed, 0};
    }
    table_ = static_cast<const std::int32_t*>(allocation);
    image_ = image;
    count_ = result.count;
    return result;
}
bool quest_mapping_references_active(const PreparedQuestMapping& prepared) noexcept {
    if (!prepared.image() || !prepared.table() || prepared.redirect_count() != reference_count) return false;
    if (prepared.borrowed()) return prepared.borrowed_references_active();
    for (std::size_t n = 0; n < reference_count; ++n) {
        const auto& edit = prepared.redirects()[n];
        std::uint8_t live[4]{};
        if (!read_exact(prepared.image()+edit.displacement_rva, live, sizeof(live)) ||
            std::memcmp(live, edit.after, sizeof(live))) return false;
    }
    return true;
}
}
