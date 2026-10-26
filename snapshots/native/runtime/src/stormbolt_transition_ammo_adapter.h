#pragma once
#include "stormbolt_instance_store.h"
#include <cstring>

namespace stormslinger {
struct StormboltTransitionSaveApi {
    void** manager_slot = nullptr; // image+5013950
    void* (*current_character)(void* manager) = nullptr; // image+1B8DBB0
    void (*set_character_bit)(void* character, std::int32_t bit) = nullptr; // 1360DA0
    bool complete() const noexcept {
        return manager_slot && current_character && set_character_bit;
    }
};
enum class StormboltTransitionAmmoResult {
    completed, rejected_before_start, invalidated_after_native
};

// Internal component for PS3A1FCCD..3A1FD79, not an installed action hook.
// Caller authenticates API/code/types, borrows weapon/owner/parameter lifetimes,
// serializes action access, and consumes event190 bit1 before invoking once.
// Native calls cannot run under the sidecar lock. A token is not a native
// lifetime guarantee. No result permits replaying an already consumed event.
template<std::size_t Capacity>
class StormboltTransitionAmmoAdapter {
    StormboltInstanceStore<Capacity>& store_;
    const StormboltTransitionSaveApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{};
        std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
        return value;
    }
    template<class T> static void write(void* object, std::size_t offset, T value) {
        std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
    }
    static void select_ammo(void* weapon) {
        write(weapon, 0x2350, std::uint8_t{1});
        write(weapon, 0x27C8, std::uint32_t{40});
    }
public:
    StormboltTransitionAmmoAdapter(StormboltInstanceStore<Capacity>& store,
        const StormboltTransitionSaveApi& api) : store_(store), api_(api) { }

    StormboltTransitionAmmoResult apply(void* weapon, StormboltInstanceToken token) {
        StormboltStageState ignored{};
        if (!api_.complete() || token.address != reinterpret_cast<std::uintptr_t>(weapon) ||
            !store_.snapshot(token, ignored))
            return StormboltTransitionAmmoResult::rejected_before_start;

        auto* owner = read<void*>(weapon, 0x9B0);
        if (!(read<std::uint8_t>(owner, 0x8938) & 1)) {
            select_ammo(weapon);
            return StormboltTransitionAmmoResult::completed;
        }

        auto* character = api_.current_character(*api_.manager_slot);
        // Read selection at call time. Invoke the game's locked bit setter;
        // do not copy its raw save layout or substitute ordinary mode's bit12.
        if (!store_.snapshot(token, ignored))
            return StormboltTransitionAmmoResult::invalidated_after_native;
        api_.set_character_bit(character, 11);
        if (!store_.snapshot(token, ignored))
            return StormboltTransitionAmmoResult::invalidated_after_native;

        // PS re-reads the owner and gate after saving, before writing ammo.
        owner = read<void*>(weapon, 0x9B0);
        const bool reset = (read<std::uint8_t>(owner, 0x8938) & 1) != 0;
        select_ammo(weapon);
        if (!reset) return StormboltTransitionAmmoResult::completed;

        const auto* parameter = read<void*>(owner, 0x7810);
        const auto timer_bits = read<std::uint32_t>(parameter, 0x1608);
        const bool applied = store_.transact(token, [timer_bits](StormboltStageState& state) noexcept {
            state.counter = 0;
            // Exact DWORD copy in PS; preserve even unusual parameter bit patterns.
            std::memcpy(&state.action_timer, &timer_bits, sizeof(timer_bits));
            // This transition does not reset shot_snapshot or decay_timer.
            return true;
        });
        return applied ? StormboltTransitionAmmoResult::completed :
            StormboltTransitionAmmoResult::invalidated_after_native;
    }
};
} // namespace stormslinger
