#include "horizon_mapping_transaction.h"
#include "horizon_owner_verification.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
namespace horizon_other {
namespace {
// Saved PC menu owner includes its local switch tables. Only group 123 is
// removed by the JE at 1F3A965; restore its original status-bit query path.
constexpr TreeOwnerSpec kTutorialMenuOwner = {0x1F3A870, 985,
    {0xDE, 0x1B, 0x9D, 0x07, 0x8C, 0x95, 0xDD, 0x1E,
     0x25, 0xB5, 0xA0, 0x3E, 0x9C, 0x2E, 0x77, 0xA8,
     0x96, 0xF8, 0x20, 0x53, 0x43, 0x8C, 0x1B, 0xF2,
     0xC3, 0x46, 0xD7, 0x6A, 0xB5, 0xA2, 0xDF, 0x87}};
// Reuse the existing four-byte ownership/write/restore slot. The following
// MOV prefix stays byte-identical; only the two-byte JE becomes two NOPs.
constexpr QuestMappingRedirect kTutorialMenuEdit = {
    0x1F3A965, {0x74, 0x43, 0x48, 0x8B}, {0x90, 0x90, 0x48, 0x8B}};
bool matches(const std::uint8_t* address, const std::uint8_t* expected) noexcept {
    std::uint8_t bytes[4]{};
    SIZE_T count = 0;
    return ReadProcessMemory(GetCurrentProcess(), address, bytes, 4, &count) && count == 4 &&
           std::memcmp(bytes, expected, 4) == 0;
}
}
bool QuestMappingTransaction::active() const noexcept {
    for (std::size_t n = 0; n < count_; ++n)
        if (slots_[n].owned || slots_[n].protection_pending || slots_[n].flush_pending) return true;
    return false;
}
bool QuestMappingTransaction::maintain(Slot& slot) noexcept {
    bool complete = true;
    if (slot.flush_pending) {
        if (FlushInstructionCache(GetCurrentProcess(), slot.address, 4)) slot.flush_pending = false;
        else complete = false;
    }
    if (slot.protection_pending) {
        DWORD previous = 0;
        if (VirtualProtect(slot.address, 4, slot.protection, &previous)) slot.protection_pending = false;
        else complete = false;
    }
    return complete;
}
bool QuestMappingTransaction::write(Slot& slot, bool install) noexcept {
    if (!maintain(slot) || !matches(slot.address, install ? slot.before : slot.after)) return false;
    DWORD previous = 0;
    if (!VirtualProtect(slot.address, 4, PAGE_EXECUTE_READWRITE, &previous)) return false;
    slot.protection = previous;
    slot.protection_pending = true;
    std::memcpy(slot.address, install ? slot.after : slot.before, 4);
    slot.owned = install;
    slot.flush_pending = true;
    return maintain(slot);
}
bool QuestMappingTransaction::restore_while_quiescent() noexcept {
    bool complete = true;
    for (std::size_t n = count_; n > 0; --n) {
        auto& slot = slots_[n-1];
        if (!maintain(slot) || (slot.owned && !write(slot, false))) complete = false;
    }
    return complete && !active();
}
MappingInstallResult QuestMappingTransaction::install_while_quiescent(std::uint8_t* image,
    const PreparedQuestMapping& prepared) noexcept {
    const auto quest_count = prepared.redirect_count();
    if (!image || image != prepared.image() || !prepared.table() || !quest_count ||
        quest_count >= sizeof(slots_)/sizeof(slots_[0]) || active())
        return MappingInstallResult::invalid_state;
    if (!verify_native_owner(image, kTutorialMenuOwner))
        return MappingInstallResult::original_mismatch;
    const auto count = quest_count + 1;
    // Verify every quest displacement and the tutorial branch before any write.
    count_ = count;
    for (std::size_t n = 0; n < count; ++n) {
        const auto& edit = n < quest_count ? prepared.redirects()[n] : kTutorialMenuEdit;
        auto& slot = slots_[n];
        slot = {};
        slot.address = image+edit.displacement_rva;
        std::memcpy(slot.before, edit.before, 4);
        std::memcpy(slot.after, edit.after, 4);
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(slot.address, &region, sizeof(region)) || region.State != MEM_COMMIT)
            return MappingInstallResult::original_mismatch;
        const auto offset = reinterpret_cast<std::uintptr_t>(slot.address)-reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (offset > region.RegionSize || 4 > region.RegionSize-offset || !matches(slot.address, slot.before))
            return MappingInstallResult::original_mismatch;
    }
    for (std::size_t n = 0; n < count; ++n) {
        if (!write(slots_[n], true))
            return restore_while_quiescent() ? MappingInstallResult::write_failed : MappingInstallResult::rollback_pending;
    }
    return MappingInstallResult::installed;
}
}
