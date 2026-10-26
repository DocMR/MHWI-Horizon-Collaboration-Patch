#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <filesystem>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_pages.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace fs=std::filesystem;
int main(int argc,char** argv) {
    using namespace horizon_other;
    const fs::path root=argc>1?fs::path(argv[1]):fs::path("research/horizon-complete-notice-pages-fixture-20261021");
    CHECK(!fs::exists(root) && fs::create_directory(root));
    const int primary=0,normal=0,cancel=1;
    auto state=std::make_shared<NoticeAcknowledgement>((root/"complete.version").wstring(),"v1");
    auto pages=std::make_shared<NoticePages>(state);
    NoticeNativeFunction previous;
    for(std::size_t i=0;i<6;++i) {
        CHECK(pages->ready() && pages->page()==i);
        auto callback=pages->prepare_page();CHECK(callback && !pages->ready() && !pages->prepare_page());
        if(previous)previous(primary,normal); // stale duplicate cannot skip a page
        CHECK(pages->page()==i);
        auto native_copy=callback;callback={};CHECK(!pages->ready());
        native_copy(primary,normal);previous=native_copy;
        CHECK(fs::exists(root/"complete.version")== (i==5));
    }
    CHECK(state->seen() && pages->finished() && !pages->ready());
    for(int stop=0;stop<6;++stop) {
        auto ack=std::make_shared<NoticeAcknowledgement>((root/("cancel"+std::to_string(stop))).wstring(),"v1");
        auto session=std::make_shared<NoticePages>(ack);
        for(int i=0;i<stop;++i){auto fn=session->prepare_page();fn(primary,normal);}
        auto fn=session->prepare_page();fn(cancel,normal);
        CHECK(session->finished() && !ack->seen() && !ack->pending());
        CHECK(!fs::exists(root/("cancel"+std::to_string(stop))));
    }
    auto dropped=std::make_shared<NoticeAcknowledgement>((root/"dropped").wstring(),"v1");
    auto session=std::make_shared<NoticePages>(dropped);
    auto original=session->prepare_page();auto copy=original;original={};CHECK(dropped->pending());
    copy={};CHECK(session->finished() && !dropped->pending() && !dropped->seen());
    auto known=std::make_shared<NoticeAcknowledgement>((root/"complete.version").wstring(),"v1");
    CHECK(!std::make_shared<NoticePages>(known)->prepare_page());
    std::cout<<"PASS six-page progression, final-only version persistence, duplicate callbacks, cancellation on every page, copied callback discard and seen-version suppression. No native window or input executed.\n";
}
