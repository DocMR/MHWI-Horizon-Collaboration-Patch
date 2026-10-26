#pragma once
#include <array>
#include <cstddef>
#include <string_view>

namespace stormslinger {
enum class TextFamily { none, item, lbg, series };
struct TextSelection {
    TextFamily family = TextFamily::none;
    std::size_t language = 0;
    explicit operator bool() const noexcept { return family != TextFamily::none; }
};
inline constexpr std::array<std::string_view, 14> text_languages{
    "ara", "chS", "chT", "eng", "fre", "ger", "ita", "jpn", "kor", "pol", "ptB", "rus", "spL", "spa"
};
// Caller supplies a verified readable snapshot, never an unchecked game pointer.
// Resource +0x0c is copied with a 79-byte bound in the audited acquisition path.
inline TextSelection select_text_resource(const char* snapshot, std::size_t size) noexcept {
    if (!snapshot || size == 0) return {};
    const auto limit = size < 79 ? size : 79;
    std::size_t length = 0;
    while (length < limit && snapshot[length] != '\0') ++length;
    if (length == limit) return {};
    constexpr std::string_view prefix = "common/text/steam/";
    constexpr std::array<std::string_view, 3> families{"item", "lbg", "wep_series"};
    constexpr std::array<TextFamily, 3> ids{TextFamily::item, TextFamily::lbg, TextFamily::series};
    // Normalize only the separator: retain case-sensitive exact path matching.
    std::array<char, 79> name{};
    for (std::size_t n = 0; n < length; ++n) name[n] = snapshot[n] == '\\' ? '/' : snapshot[n];
    const std::string_view path(name.data(), length);
    if (path.substr(0, prefix.size()) != prefix) return {};
    const auto tail = path.substr(prefix.size());
    for (std::size_t f = 0; f < families.size(); ++f) {
        const auto family = families[f];
        if (tail.size() != family.size() + 4 || tail.substr(0, family.size()) != family ||
            tail[family.size()] != '_') continue;
        const auto language = tail.substr(family.size() + 1);
        for (std::size_t l = 0; l < text_languages.size(); ++l)
            if (language == text_languages[l]) return {ids[f], l};
    }
    return {};
}
} // namespace stormslinger
