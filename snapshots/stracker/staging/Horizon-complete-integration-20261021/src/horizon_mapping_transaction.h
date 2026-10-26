#pragma once
#include "horizon_quest_mapping.h"
namespace horizon_other {
enum class MappingInstallResult { installed, invalid_state, original_mismatch, write_failed, rollback_pending };
// The coordinator must keep execution and other writers quiescent throughout
// install/restore and shared Hook commit/rollback. This class does not suspend threads.
class QuestMappingTransaction {
    struct Slot {
        std::uint8_t* address = nullptr;
        std::uint8_t before[4]{}, after[4]{};
        std::uint32_t protection = 0;
        bool owned = false, protection_pending = false, flush_pending = false;
    };
    // Up to 64 quest redirects plus the owned tutorial-menu branch.
    Slot slots_[65]{};
    std::size_t count_ = 0;
    static bool maintain(Slot& slot) noexcept;
    static bool write(Slot& slot, bool install) noexcept;
public:
    QuestMappingTransaction() = default;
    QuestMappingTransaction(const QuestMappingTransaction&) = delete;
    QuestMappingTransaction& operator=(const QuestMappingTransaction&) = delete;
    bool active() const noexcept;
    MappingInstallResult install_while_quiescent(std::uint8_t* certified_image,
                                                const PreparedQuestMapping& prepared,
                                                bool publish_quest_redirects=true) noexcept;
    bool restore_while_quiescent() noexcept;
};
}
