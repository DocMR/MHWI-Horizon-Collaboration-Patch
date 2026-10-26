#include <iostream>
#include <string_view>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_content.h"
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_language.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
int main() {
    using namespace horizon_other;
    constexpr std::array<std::size_t,6> expected{11,10,10,11,9,11};
    for(const auto& language:notice_localized_pages) {
        CHECK(notice_close_text(language.language));
        for(std::size_t page=0;page<6;++page) {
            const auto* ptr=notice_page_text(language.language,page);CHECK(ptr);
            const std::string_view text(ptr);CHECK(!text.empty() && text.find("HARDUMMY")==text.npos);
            std::size_t count=0,pos=0;
            while((pos=text.find("\xc2\xb7 ",pos))!=text.npos){++count;pos+=3;}
            CHECK(count==expected[page]);
        }
        CHECK(!notice_page_text(language.language,6));
    }
    CHECK(notice_language_from_native_index(8)=="chS");
    CHECK(notice_language_from_native_index(7)=="chT");
    CHECK(notice_page_text(notice_language_from_native_index(8),0));
    CHECK(!notice_page_text(notice_language_from_native_index(12),0));
    CHECK(!notice_page_text("unknown",0));
    CHECK(!notice_close_text("unknown"));
    CHECK(std::string_view(notice_close_text("chS"))=="\xe5\x85\xb3\xe9\x97\xad");
    CHECK(std::string_view(notice_close_text("chT"))=="\xe9\x97\x9c\xe9\x96\x89");
    std::cout<<"PASS14 embedded UTF-8 languages, six pages with62 original-name entries, native language selection and bounds. No game or display invoked.\n";
}
