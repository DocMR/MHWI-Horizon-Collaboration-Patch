#pragma once
#include "stormbolt_power_cell_sync_snapshot.h"
#include <array>
#include <atomic>

namespace stormslinger {
// Game-thread producer frames; different native stack identities distinguish
// nested calls. A completion consumes the generation captured during packing.
template<std::size_t Capacity>
class StormboltPowerCellSyncFrames {
    struct Frame {
        std::uintptr_t stack=0;
        StormboltPowerCellSyncToken controller{};
    };
    std::array<Frame,Capacity> frames_{};
    // Retirement may observe the count off-thread; frame mutation itself
    // remains confined to the native game-thread pack/completion protocol.
    std::atomic<unsigned> active_count_{0};
public:
    bool begin(std::uintptr_t stack,const StormboltPowerCellSyncSnapshot& snapshot) noexcept {
        if (!stack || !snapshot.controller_token) return false;
        for (const auto& frame:frames_) if (frame.stack==stack) return false;
        for (auto& frame:frames_) if (!frame.stack) {
            frame={stack,snapshot.controller_token};
            active_count_.fetch_add(1,std::memory_order_release);return true;
        }
        return false;
    }
    template<std::size_t StoreCapacity>
    bool complete(std::uintptr_t stack,std::uintptr_t controller,
        StormboltPowerCellSyncStore<StoreCapacity>& events) {
        for (auto& frame:frames_) {
            if (frame.stack!=stack || frame.controller.address!=controller) continue;
            const auto token=frame.controller;
            frame={};
            active_count_.fetch_sub(1,std::memory_order_release);
            return complete_stormbolt_power_cell_sync_controller(token,events);
        }
        return false;
    }
    bool active() const noexcept {
        return active_count_.load(std::memory_order_acquire)!=0;
    }
};
} // namespace stormslinger
