#pragma once
#include <cstdint>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace stormslinger {
struct GmdCopiedStorage {
    std::array<char, 79> resource_name{};
    std::uint64_t key_pool_address = 0, text_pool_address = 0;
    std::vector<unsigned char> records, key_pool, text_pool, text_pointers;
    std::uint32_t key_count = 0, text_count = 0;
};
struct OwnedTextKey { std::string key; std::size_t index; };
struct OwnedTextSnapshot {
    std::vector<OwnedTextKey> keys;
    std::vector<std::string> strings;
    void swap(OwnedTextSnapshot& other) noexcept { keys.swap(other.keys); strings.swap(other.strings); }
};
enum class GmdSnapshotResult { copied, rejected, out_of_memory, unexpected_exception };
inline bool copy_pool_string(const std::vector<unsigned char>& pool, std::uint64_t base,
    std::uint64_t pointer, std::string& output) {
    if (!base || pool.empty() || pool.size() > std::numeric_limits<std::uint64_t>::max() - base ||
        pointer < base || pointer - base >= pool.size()) return false;
    const auto offset = static_cast<std::size_t>(pointer - base);
    const auto* begin = reinterpret_cast<const char*>(pool.data() + offset);
    const auto* end = static_cast<const char*>(std::memchr(begin, 0, pool.size() - offset));
    if (!end) return false;
    output.assign(begin, static_cast<std::size_t>(end - begin));
    return true;
}
// Input buffers must already be copied under a verified resource lifetime/read boundary.
// Relocated pointers are validated as integers; no game pointer is dereferenced here.
inline GmdSnapshotResult decode_gmd_snapshot(const GmdCopiedStorage& input,
    OwnedTextSnapshot& output) noexcept {
    try {
        if (!input.key_count || !input.text_count ||
            input.key_count > std::numeric_limits<std::size_t>::max() / 32 ||
            input.text_count > std::numeric_limits<std::size_t>::max() / 8 ||
            input.records.size() != static_cast<std::size_t>(input.key_count) * 32 ||
            input.text_pointers.size() != static_cast<std::size_t>(input.text_count) * 8)
            return GmdSnapshotResult::rejected;
        OwnedTextSnapshot snapshot;
        snapshot.strings.resize(input.text_count);
        for (std::size_t n = 0; n < input.text_count; ++n) {
            std::uint64_t pointer = 0;
            std::memcpy(&pointer, input.text_pointers.data() + n * 8, 8);
            if (!copy_pool_string(input.text_pool, input.text_pool_address, pointer, snapshot.strings[n]))
                return GmdSnapshotResult::rejected;
        }
        snapshot.keys.reserve(input.key_count);
        for (std::size_t n = 0; n < input.key_count; ++n) {
            std::uint32_t index = 0;
            std::uint64_t pointer = 0;
            std::memcpy(&index, input.records.data() + n * 32, 4);
            std::memcpy(&pointer, input.records.data() + n * 32 + 16, 8);
            index &= 0x7fffffff;
            OwnedTextKey key{{}, index};
            if (index >= input.text_count ||
                !copy_pool_string(input.key_pool, input.key_pool_address, pointer, key.key) || key.key.empty())
                return GmdSnapshotResult::rejected;
            for (const auto& previous : snapshot.keys)
                if (previous.key == key.key) return GmdSnapshotResult::rejected;
            snapshot.keys.push_back(std::move(key));
        }
        output.swap(snapshot);
        return GmdSnapshotResult::copied;
    } catch (const std::bad_alloc&) { return GmdSnapshotResult::out_of_memory; }
    catch (...) { return GmdSnapshotResult::unexpected_exception; }
}
} // namespace stormslinger
