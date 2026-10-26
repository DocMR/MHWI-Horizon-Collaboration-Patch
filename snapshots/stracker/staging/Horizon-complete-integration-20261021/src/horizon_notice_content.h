#pragma once
#include <array>
#include <string_view>
#include <cstddef>
namespace horizon_other {
struct NoticeLocalizedPages {
    std::string_view language;
    std::array<const char*,6> pages;
};
struct NoticeLocalizedClose {
    std::string_view language;
    const char* text;
};
#include "generated_notice_page_text.inc"
// Static DLL storage survives every deferred native callback while the host
// retains its normal process-lifetime module ownership.
inline const char* notice_page_text(std::string_view language,std::size_t page) noexcept {
    if(page>=6)return nullptr;
    for(const auto& copy:notice_localized_pages)
        if(copy.language==language)return copy.pages[page];
    return nullptr;
}
inline const char* notice_close_text(std::string_view language) noexcept {
    for(const auto& copy:notice_localized_close)
        if(copy.language==language)return copy.text;
    return nullptr;
}
} // namespace horizon_other
