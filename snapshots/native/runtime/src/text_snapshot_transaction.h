#pragma once
#include "gmd_snapshot.h"
#include "text_prepare_transaction.h"

namespace stormslinger {
// Override keys must come from independently verified source/priority information.
// Snapshot content alone cannot prove whether a baseline-valued MOD override exists.
inline TextPrepareResult prepare_snapshot_transaction(const TextDeltaGroup& group,
    const OwnedTextSnapshot& snapshot, const std::vector<std::string_view>& override_keys,
    TextAllocator allocator, PreparedTextPool& output) noexcept {
    try {
        std::vector<TextKeySnapshot> keys;
        std::vector<std::string_view> strings;
        keys.reserve(snapshot.keys.size()); strings.reserve(snapshot.strings.size());
        for (const auto& value : snapshot.strings) strings.emplace_back(value);
        for (const auto& key : snapshot.keys) keys.push_back({key.key, key.index, false});
        for (std::size_t n = 0; n < override_keys.size(); ++n) {
            if (override_keys[n].empty()) return TextPrepareResult::rejected;
            for (std::size_t j = 0; j < n; ++j)
                if (override_keys[j] == override_keys[n]) return TextPrepareResult::rejected;
            bool found = false;
            for (auto& key : keys) if (key.key == override_keys[n]) {
                key.explicit_mod_override = true;
                found = true;
            }
            if (!found) return TextPrepareResult::rejected;
        }
        return prepare_text_transaction(group, keys, strings, allocator, output);
    } catch (const std::bad_alloc&) { return TextPrepareResult::out_of_memory; }
    catch (...) { return TextPrepareResult::unexpected_exception; }
}
} // namespace stormslinger
