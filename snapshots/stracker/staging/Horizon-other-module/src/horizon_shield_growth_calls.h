#pragma once
#include "../../../runtime/src/stormbolt_growth_session.h"
#include "../../../runtime/src/hooks.h"
namespace horizon_other {
// Equipment growth only. The unified owner supplies ONE shared set of status,
// damage, combine-loop and sync stores and all four authenticated lifecycles.
// Never prepare alongside the full session's same three growth callsites.
template<std::size_t Capacity, std::size_t LoopCapacity>
class ShieldGrowthCalls {
    stormslinger::StormboltGrowthSession<Capacity,LoopCapacity> growth_;
    stormslinger::HookTransaction* transaction_ = nullptr;
    bool attempted_ = false, prepared_ = false, activated_ = false, image_present_ = false;
    static std::array<std::uintptr_t,3> sites(const std::uint8_t* image) noexcept {
        if (!image) return {};
        const auto base = reinterpret_cast<std::uintptr_t>(image);
        return {{base+0x121B37F, base+0x121B39E, base+0x121B3A7}};
    }
public:
    ShieldGrowthCalls(const std::uint8_t* certified_image,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        stormslinger::StormboltDamageExceptionStore<Capacity>& damages,
        stormslinger::StormboltPowerCellLoopStore<LoopCapacity>& loops,
        stormslinger::StormboltPowerCellSyncStore<Capacity>& sync,
        const stormslinger::StormboltPowerCellCompleteGrowthBindings& bindings)
        : growth_(sites(certified_image), statuses, damages, loops, sync, bindings),
          image_present_(certified_image != nullptr) {}
    ShieldGrowthCalls(const ShieldGrowthCalls&) = delete;
    ShieldGrowthCalls& operator=(const ShieldGrowthCalls&) = delete;
    // Host certifies image, native fields/APIs/resources and event boundaries.
    // Keep execution/other writers quiescent throughout prepare/commit/activate.
    bool prepare_while_quiescent(stormslinger::HookTransaction& transaction) {
        if (attempted_ || !image_present_ || !transaction.can_add_hooks() ||
            stormslinger::stormbolt_growth_dispatch_target) return false;
        attempted_ = true;
        transaction_ = &transaction;
        prepared_ = growth_.prepare(&transaction);
        return prepared_;
    }
    bool activate_after_shared_commit_while_quiescent() {
        if (activated_ || !prepared_ || !transaction_ || !transaction_->has_library_state() ||
            !transaction_->last.active) return false;
        if (growth_.activate()) { activated_ = true; return true; }
        (void)restore_while_quiescent();
        return false;
    }
    // On failure keep stores/DLL/bridges and retry restoration. Do not stop
    // shared native lifecycle hooks while any growth CALL remains installed.
    bool restore_while_quiescent() {
        if (!growth_.restore()) return false;
        prepared_ = false;
        activated_ = false;
        return true;
    }
    bool restoration_pending() const noexcept { return growth_.active(); }
    bool release_after_shared_stop() { return growth_.release_after_shared_stop(); }
};
}
