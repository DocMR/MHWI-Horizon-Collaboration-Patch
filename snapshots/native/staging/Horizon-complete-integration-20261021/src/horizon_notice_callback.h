#pragma once
#include "horizon_notice_acknowledgement.h"
#include <functional>
#include <memory>

namespace horizon_other {
// Same MSVC function ABI already used by the native HSS callbacks. The two
// native message-result arguments are pointers to32-bit values, not values.
using NoticeNativeFunction=std::function<void(const int&,const int&)>;
static_assert(sizeof(NoticeNativeFunction)==0x40);
using NoticeCompletionObserver=void (*)(const NoticeCompletion&) noexcept;

namespace notice_detail {
struct CallbackLease {
    std::shared_ptr<NoticeAcknowledgement> state;
    NoticeCompletionObserver observer=nullptr;
    bool armed=false,invoked=false;
    ~CallbackLease() {
        // Native requests can be discarded without invoking their callback.
        // Only the final copied callable ends an undelivered request.
        if(armed && !invoked) {
            const auto completion=state->complete(0,2);
            if(completion.consumed && observer)observer(completion);
        }
    }
};
}

// The host authenticates the native message copy/invoke/disposal ABI, submits
// only while GUI-ready, and keeps DLL code alive until every copied callback is
// disposed. This creates no window and contains no authored/localized text.
inline NoticeNativeFunction make_notice_callback(
    const std::shared_ptr<NoticeAcknowledgement>& state,
    NoticeCompletionObserver observer=nullptr) {
    if(!state || !state->should_offer())return {};
    auto lease=std::make_shared<notice_detail::CallbackLease>();
    lease->state=state;lease->observer=observer;
    NoticeNativeFunction callback=[lease](const int& selected,const int& result) {
        lease->invoked=true;
        const auto completion=lease->state->complete(selected,result);
        if(completion.consumed && lease->observer)lease->observer(completion);
    };
    // All allocations precede the pending-state change.
    if(!state->begin_request())return {};
    lease->armed=true;
    return callback;
}
} // namespace horizon_other
