#pragma once
#include "horizon_notice_pages.h"
#include "horizon_notice_content.h"
#include "horizon_notice_language.h"
#include "horizon_notice_request.h"
namespace horizon_other {
struct NoticeTitleSnapshot {
    bool before_title_request=false;
    bool message_queue_ready=false;
    std::uint32_t native_language=0;
    NoticeRequestDefaults defaults;
};
// Snapshot/submit are supplied by the authenticated native host. Submission
// must copy the request before returning, as original1AE2050 does. It creates
// no independent window and calls no original aLogo update itself.
class NoticeTitleFlow {
    std::shared_ptr<NoticePages> pages_;
public:
    using Submit=void(*)(void*,const NoticeRequest&);
    explicit NoticeTitleFlow(std::shared_ptr<NoticePages> pages):pages_(std::move(pages)) {}
    bool hold_title(const NoticeTitleSnapshot& snapshot,Submit submit,void* context) {
        if(!snapshot.before_title_request || !pages_ || !pages_->needed())return false;
        if(!snapshot.message_queue_ready || !submit || !pages_->ready())return true;
        const auto language=notice_language_from_native_index(snapshot.native_language);
        const auto* text=notice_page_text(language,pages_->page());
        // The native host must resolve a supported game language before
        // requesting a page; never substitute a guessed Windows language.
        if(!text)return true;
        auto callback=pages_->prepare_page();
        if(callback) {
            auto request=make_notice_request(snapshot.defaults,text,std::move(callback));
            submit(context,request);
        }
        return pages_->needed();
    }
};
} // namespace horizon_other
