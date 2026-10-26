#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include "../../../runtime/src/text_resource_selector.h"

namespace horizon_other {
// Native text-path rewrite reads GUI manager +4C for mode0; indices come from
// the original 4BE4420 language-code table, not Windows locale or our14-array.
// Caller authenticates and snapshots the live field before using this helper.
inline constexpr std::string_view notice_language_from_native_index(std::uint32_t index) noexcept {
    constexpr std::array<std::string_view,24> native{
        "jpn","eng","fre","spa","ger","ita","kor","chT","chS","por","rus","pol",
        "dut","fin","swe","dan","nor","cze","hun","slo","oth","ptB","ara","spL"};
    if(index>=native.size())return {};
    for(const auto supported:stormslinger::text_languages)
        if(native[index]==supported)return supported;
    return {}; // An unsupported legacy index is not silently shown as English.
}
} // namespace horizon_other
