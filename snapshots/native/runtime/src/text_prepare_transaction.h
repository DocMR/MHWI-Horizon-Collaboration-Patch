#pragma once
#include "prepared_text_pool.h"
#include <new>

namespace stormslinger {
enum class TextPrepareResult { prepared, rejected, out_of_memory, unexpected_exception };
// Snapshot views and resource allocator context must remain alive through this call.
// No game fields are published here; this boundary catches C++ exceptions only.
inline TextPrepareResult prepare_text_transaction(const TextDeltaGroup& group,
    const std::vector<TextKeySnapshot>& keys, const std::vector<std::string_view>& strings,
    TextAllocator allocator, PreparedTextPool& output) noexcept {
    try {
        std::vector<TextMergeEdit> plan;
        if (!plan_text_merge(group, keys, strings, plan)) return TextPrepareResult::rejected;
        if (!prepare_text_pool(strings, plan, allocator, output)) return TextPrepareResult::rejected;
        return TextPrepareResult::prepared;
    } catch (const std::bad_alloc&) {
        return TextPrepareResult::out_of_memory;
    } catch (...) {
        return TextPrepareResult::unexpected_exception;
    }
}
} // namespace stormslinger
