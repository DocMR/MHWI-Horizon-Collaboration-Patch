#pragma once
#include "../../../runtime/src/stormbolt_owned_call_site.h"
#include <optional>

namespace horizon_other {
// Borrow the host transaction; publication/restore require the same drained,
// quiescent interval as the existing native equipment call sites.
class TutorialTriggerCall {
    const std::uint8_t* image_ = nullptr;
    std::optional<stormslinger::StormboltOwnedReplayCallSite<7>> call_;
    bool attempted_ = false, bindings_published_ = false;
public:
    bool prepare_while_quiescent(const std::uint8_t* image,
                                 stormslinger::HookTransaction& transaction);
    bool activate_after_shared_commit_while_quiescent();
    bool restore_while_quiescent();
    bool release_after_shared_stop_while_quiescent();
};
}
