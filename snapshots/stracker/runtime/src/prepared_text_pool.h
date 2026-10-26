#pragma once
#include "text_merge_plan.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

namespace stormslinger {
struct TextAllocator {
    void* context = nullptr;
    void* (*allocate)(void*, std::size_t) noexcept = nullptr;
    void (*release)(void*, void*) noexcept = nullptr;
};
struct TextPoolReceipt {
    char* pool = nullptr;
    char** pointers = nullptr;
    std::uint32_t bytes = 0;
    std::size_t count = 0;
    TextAllocator allocator{};
};
class PreparedTextPool {
public:
    TextAllocator allocator{};
    char* pool = nullptr;
    char** pointers = nullptr;
    std::uint32_t bytes = 0;
    std::size_t count = 0;
    PreparedTextPool() = default;
    PreparedTextPool(const PreparedTextPool&) = delete;
    PreparedTextPool& operator=(const PreparedTextPool&) = delete;
    ~PreparedTextPool() { reset(); }
    void reset() noexcept {
        if (pointers) allocator.release(allocator.context, pointers);
        if (pool) allocator.release(allocator.context, pool);
        pool = nullptr; pointers = nullptr; bytes = 0; count = 0;
    }
    void swap(PreparedTextPool& other) noexcept {
        std::swap(allocator, other.allocator); std::swap(pool, other.pool);
        std::swap(pointers, other.pointers); std::swap(bytes, other.bytes); std::swap(count, other.count);
    }
    TextPoolReceipt view() const noexcept { return {pool, pointers, bytes, count, allocator}; }
    // Called only after the resource commit confirms these exact buffers and ownership.
    // This does not publish fields or establish exclusion from readers by itself.
    bool relinquish_after_commit(const TextPoolReceipt& committed) noexcept {
        if (!pool || !pointers || !bytes || !count || committed.pool != pool ||
            committed.pointers != pointers || committed.bytes != bytes || committed.count != count ||
            committed.allocator.context != allocator.context ||
            committed.allocator.allocate != allocator.allocate || committed.allocator.release != allocator.release)
            return false;
        pool = nullptr; pointers = nullptr; bytes = 0; count = 0;
        allocator = {};
        return true;
    }
};
// Ownership stays here until a separate, verified game-resource commit is implemented.
inline bool prepare_text_pool(const std::vector<std::string_view>& strings,
    const std::vector<TextMergeEdit>& edits, TextAllocator allocator, PreparedTextPool& output) {
    if (!allocator.allocate || !allocator.release || strings.empty() ||
        strings.size() > std::numeric_limits<std::uint32_t>::max() / sizeof(char*)) return false;
    auto values = strings;
    for (std::size_t n = 0; n < edits.size(); ++n) {
        const auto& edit = edits[n];
        if (edit.index >= strings.size() || strings[edit.index] != edit.expected) return false;
        for (std::size_t j = 0; j < n; ++j) if (edits[j].index == edit.index) return false;
        if (edit.status != TextMergeStatus::update && edit.replacement != edit.expected) return false;
        values[edit.index] = edit.replacement;
    }
    std::size_t total = 0;
    for (const auto value : values) {
        if (value.find('\0') != std::string_view::npos ||
            value.size() >= std::numeric_limits<std::uint32_t>::max() - total) return false;
        total += value.size() + 1;
    }
    PreparedTextPool prepared;
    prepared.allocator = allocator;
    prepared.pool = static_cast<char*>(allocator.allocate(allocator.context, total));
    if (!prepared.pool) return false;
    prepared.pointers = static_cast<char**>(allocator.allocate(allocator.context, strings.size() * sizeof(char*)));
    if (!prepared.pointers) return false;
    prepared.bytes = static_cast<std::uint32_t>(total);
    prepared.count = values.size();
    std::size_t offset = 0;
    for (std::size_t n = 0; n < values.size(); ++n) {
        prepared.pointers[n] = prepared.pool + offset;
        if (!values[n].empty()) std::memcpy(prepared.pool + offset, values[n].data(), values[n].size());
        offset += values[n].size();
        prepared.pool[offset++] = '\0';
    }
    output.swap(prepared);
    return true;
}
} // namespace stormslinger
