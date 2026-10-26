#pragma once
#include "horizon_notice_callback.h"
#include <cstddef>

namespace horizon_other {
// Serialized GUI-thread state only. The host submits each requested page on a
// later GUI update, after the prior window's callback/close has returned.
class NoticePages : public std::enable_shared_from_this<NoticePages> {
    std::shared_ptr<NoticeAcknowledgement> acknowledgement_;
    NoticeCompletionObserver observer_;
    std::size_t page_=0;
    bool started_=false,in_flight_=false,finished_=false;
    struct Lease {
        std::shared_ptr<NoticePages> owner;
        std::size_t page=0;
        bool armed=false,invoked=false;
        ~Lease() {if(armed && !invoked)owner->complete(page,0,2);}
    };
    void complete(std::size_t page,int selected,int result) noexcept {
        if(!in_flight_ || finished_ || page!=page_)return;
        in_flight_=false;
        if(selected==0 && result==0 && page_+1<page_count) {
            ++page_;return;
        }
        finished_=true;
        const auto completion=acknowledgement_->complete(selected,result);
        if(completion.consumed && observer_)observer_(completion);
    }
public:
    static constexpr std::size_t page_count=6;
    NoticePages(std::shared_ptr<NoticeAcknowledgement> state,
                NoticeCompletionObserver observer=nullptr)
        :acknowledgement_(std::move(state)),observer_(observer) {}
    std::size_t page() const noexcept {return page_;}
    bool finished() const noexcept {return finished_;}
    bool needed() const noexcept {
        return acknowledgement_ && !finished_ && !acknowledgement_->seen() &&
            (started_ || acknowledgement_->should_offer());
    }
    bool ready() const noexcept {
        return acknowledgement_ && !finished_ && !in_flight_ &&
            (started_?acknowledgement_->pending():acknowledgement_->should_offer());
    }
    NoticeNativeFunction prepare_page() {
        if(!ready())return {};
        // Allocate before changing either session's pending state. Copies of
        // this callback keep the session alive without storing a cycle in it.
        auto lease=std::make_shared<Lease>();
        lease->owner=shared_from_this();lease->page=page_;
        NoticeNativeFunction callback=[lease](const int& selected,const int& result) {
            if(lease->invoked)return;
            lease->invoked=true;
            lease->owner->complete(lease->page,selected,result);
        };
        if(!started_) {
            if(!acknowledgement_->begin_request())return {};
            started_=true;
        }
        in_flight_=lease->armed=true;
        return callback;
    }
};
} // namespace horizon_other
