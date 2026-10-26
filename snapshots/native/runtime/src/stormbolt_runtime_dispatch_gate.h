#pragma once
#include "stormbolt_special_to_normal_hook.h"
#include <atomic>
namespace stormslinger {
// Coordinates publication only; native lifetime/quiescence is still required.
class StormboltRuntimeDispatchGate {
    enum class State { dormant,enabled,restoring };
    std::atomic<State> state_{State::dormant};
public:
    void enable() {state_.store(State::enabled,std::memory_order_release);}
    void begin_restore() {
        auto expected=State::enabled;
        state_.compare_exchange_strong(expected,State::restoring,std::memory_order_acq_rel);
    }
    void finish_restore() {state_.store(State::dormant,std::memory_order_release);}
    bool weapon() const {return state_.load(std::memory_order_acquire)==State::enabled;}
    bool action(StormboltSpecialActionPhase phase) const {
        const auto state=state_.load(std::memory_order_acquire);
        return state==State::enabled || (state==State::restoring && phase==StormboltSpecialActionPhase::exit);
    }
};
}
