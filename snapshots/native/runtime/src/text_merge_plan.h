#pragma once
#include "generated_text_deltas.h"
#include <vector>

namespace stormslinger {
struct TextKeySnapshot { std::string_view key; std::size_t index; bool explicit_mod_override = false; };
enum class TextMergeStatus { update, already_updated, external_override };
struct TextMergeEdit { std::size_t index; std::string_view expected, replacement; TextMergeStatus status; };
// Borrowed views are valid only while the caller holds the snapshot's lifetime.
// Planning never mutates game storage. Publishing requires separate ownership/locking.
inline bool plan_text_merge(const TextDeltaGroup& group,
    const std::vector<TextKeySnapshot>& keys, const std::vector<std::string_view>& strings,
    std::vector<TextMergeEdit>& output) {
    if (group.start > text_deltas.size() || group.count > text_deltas.size() - group.start) return false;
    for (std::size_t k = 0; k < keys.size(); ++k) {
        if (keys[k].key.empty() || keys[k].key.find('\0') != std::string_view::npos || keys[k].index >= strings.size()) return false;
        for (std::size_t j = 0; j < k; ++j) if (keys[j].key == keys[k].key) return false;
    }
    std::vector<TextMergeEdit> prepared;
    prepared.reserve(group.count);
    for (std::size_t d = group.start; d < group.start + group.count; ++d) {
        const auto& delta = text_deltas[d];
        if (!delta.key_count || delta.key_start > text_delta_keys.size() ||
            delta.key_count > text_delta_keys.size() - delta.key_start) return false;
        std::size_t index = strings.size();
        bool explicit_override = false;
        for (std::size_t n = 0; n < delta.key_count; ++n) {
            const auto target = text_delta_keys[delta.key_start + n];
            const TextKeySnapshot* found = nullptr;
            for (const auto& key : keys) if (key.key == target) found = &key;
            if (!found || (index != strings.size() && index != found->index)) return false;
            index = found->index;
            explicit_override = explicit_override || found->explicit_mod_override;
        }
        // Every alias of the selected index must belong to this delta.
        for (const auto& key : keys) if (key.index == index) {
            bool allowed = false;
            for (std::size_t n = 0; n < delta.key_count; ++n)
                allowed = allowed || key.key == text_delta_keys[delta.key_start + n];
            if (!allowed) return false;
        }
        for (const auto& edit : prepared) if (edit.index == index) return false;
        const auto value = strings[index];
        if (value.find('\0') != std::string_view::npos) return false;
        const auto status = explicit_override ? TextMergeStatus::external_override :
            value == delta.before ? TextMergeStatus::update :
            value == delta.after ? TextMergeStatus::already_updated : TextMergeStatus::external_override;
        prepared.push_back({index, value, status == TextMergeStatus::update ? delta.after : value, status});
    }
    output.swap(prepared);
    return true;
}
} // namespace stormslinger
