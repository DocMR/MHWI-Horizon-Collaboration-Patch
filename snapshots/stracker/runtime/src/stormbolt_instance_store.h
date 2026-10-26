#pragma once
#include "stormbolt_stage_core.h"
#include <limits>
#include <mutex>
#include <type_traits>

namespace stormslinger {
struct StormboltInstanceToken {
    std::uintptr_t address = 0;
    std::uint64_t generation = 0;
    explicit operator bool() const noexcept { return address && generation; }
};
// Sidecar only: native hooks must establish construction/destruction ordering.
// The lock protects sidecar state, not the lifetime of any native game pointer.
// Operations run synchronously under the lock: no recursion or native callbacks.
template<std::size_t Capacity, class State=StormboltStageState, class Token=StormboltInstanceToken>
class StormboltInstanceStore {
    static_assert(Capacity > 0, "Instance capacity must be positive");
    struct Entry { Token token{}; State state{}; };
    std::array<Entry, Capacity> entries_{};
    std::uint64_t generation_ = 0;
    std::mutex mutex_;
    Entry* find(Token token) noexcept {
        if (!token) return nullptr;
        for (auto& entry : entries_)
            if (entry.token.address == token.address &&
                entry.token.generation == token.generation) return &entry;
        return nullptr;
    }
public:
    // Only use from a native callback that already borrows the live object.
    // Lookup does not establish native lifetime and never creates missing state.
    bool token_for_live_address(std::uintptr_t address, Token& output) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!address) return false;
        for (const auto& entry : entries_)
            if (entry.token.address == address && entry.token) {
                output = entry.token;
                return true;
            }
        return false;
    }
    // Call after either successful native initialization. Reinitialization resets
    // state and invalidates old tokens, even if the address did not change.
    Token initialize(std::uintptr_t address) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!address || generation_ == (std::numeric_limits<std::uint64_t>::max)())
            return {};
        Entry* destination = nullptr;
        for (auto& entry : entries_) {
            if (entry.token.address == address) { destination = &entry; break; }
            if (!entry.token && !destination) destination = &entry;
        }
        if (!destination) return {}; // No eviction of an active instance.
        *destination = Entry{Token{address, ++generation_}, {}};
        return destination->token;
    }
    bool retire(Token token) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* entry = find(token);
        if (!entry) return false;
        *entry = {};
        return true;
    }
    bool snapshot(Token token, State& output) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* entry = find(token);
        if (!entry) return false;
        output = entry->state;
        return true;
    }
    template<class Operation>
    bool transact(Token token, Operation&& operation) {
        static_assert(std::is_nothrow_invocable_r_v<bool, Operation, State&>,
            "State operation must return bool without throwing");
        std::lock_guard<std::mutex> lock(mutex_);
        auto* entry = find(token);
        if (!entry) return false; // Do not invoke operation on a stale token.
        auto candidate = entry->state;
        if (!operation(candidate)) return false;
        entry->state = candidate;
        return true;
    }
};
} // namespace stormslinger
