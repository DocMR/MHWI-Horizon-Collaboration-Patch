#pragma once
#include "generated_stormbolt_motion_events.h"
#include <array>
#include <cstddef>
#include <cstring>

namespace stormslinger {
// Native timeline players retain key pointers. Keep this owner alive until all
// such players stop using them; the production binding will use process lifetime.
class StormboltMotionEvents {
    alignas(8) std::array<std::uint8_t,sizeof(stormbolt_motion_event_bytes)> bytes_{};
public:
    StormboltMotionEvents() noexcept {
        std::memcpy(bytes_.data(),stormbolt_motion_event_bytes,bytes_.size());
        for(const auto offset:stormbolt_motion_event_relocations) {
            std::uint64_t target{};
            std::memcpy(&target,bytes_.data()+offset,sizeof(target));
            const auto pointer=reinterpret_cast<std::uintptr_t>(bytes_.data()+target);
            std::memcpy(bytes_.data()+offset,&pointer,sizeof(pointer));
        }
    }
    StormboltMotionEvents(const StormboltMotionEvents&)=delete;
    StormboltMotionEvents& operator=(const StormboltMotionEvents&)=delete;
    bool select(std::uint32_t slot,const void*& events) const noexcept {
        for(const auto& root:stormbolt_motion_event_roots)
            if(root.slot==slot) {
                events=root.offset==0xFFFFFFFFu ? nullptr : bytes_.data()+root.offset;
                return true;
            }
        return false;
    }
};
} // namespace stormslinger
