#pragma once
#include "stormbolt_evade_reload_effect.h"
#include "stormbolt_owned_call_site.h"
#include "stormbolt_callback_publication.h"
#include <memory>

namespace stormslinger {
// Prepare/activate/restore require native execution quiescent. Shared shooting
// transaction must restore this CALL before destroying native dependencies.
template<std::size_t Capacity>
class StormboltEvadeReloadEffectSession {
    struct State {
        StormboltEvadeReloadEffect<Capacity> adapter;
        StormboltOwnedCallSite patch;
        bool retired=false;
        std::uintptr_t site;
        std::array<unsigned char,5> expected;
        State(StormboltInstanceStore<Capacity>& instances,StormboltEvadeReloadEffectBindings api,
            std::uintptr_t address,std::array<unsigned char,5> bytes,HookTransaction* transaction)
            :adapter(instances,api),patch(transaction),site(address),expected(bytes) {}
    };
    std::unique_ptr<State> state_;
    static inline StormboltCallbackPublication<State> publication_;
    static void entry(void* weapon) {
        auto lease=publication_.acquire();
        if(lease.owner) lease.owner->adapter.invoke(weapon);
    }
public:
    StormboltEvadeReloadEffectSession()=default;
    StormboltEvadeReloadEffectSession(const StormboltEvadeReloadEffectSession&)=delete;
    ~StormboltEvadeReloadEffectSession() {
        if(state_ && !restore()) state_.release(); // Keep published callback owner on conflict.
    }
    bool active() const noexcept {return state_!=nullptr;}
    bool prepare(StormboltInstanceStore<Capacity>& instances,std::uintptr_t site,
        StormboltEvadeReloadEffectBindings api,const std::array<unsigned char,5>& expected,
        HookTransaction* transaction=nullptr) {
        if(state_ || !site || !api.original_refill || !api.weapon_effect || !api.ready ||
            !api.resolved_model_offset || expected[0]!=0xE8) return false;
        std::array<unsigned char,5> current{};
        if(!read_stormbolt_field(site,0,current) || current!=expected) return false;
        std::int32_t displacement=0;std::memcpy(&displacement,expected.data()+1,4);
        const auto target=site+5+static_cast<std::intptr_t>(displacement);
        if(target!=reinterpret_cast<std::uintptr_t>(api.original_refill)) return false;
        auto candidate=std::make_unique<State>(instances,api,site,expected,transaction);
        if(!publication_.publish(candidate.get())) return false;
        state_=std::move(candidate);
        return state_->patch.prepare(reinterpret_cast<void*>(state_->site),
            reinterpret_cast<std::uintptr_t>(&entry),state_->expected);
    }
    bool activate() {
        return state_ && state_->patch.activate();
    }
    // Revoke the CALL while retaining its relay and callback owner.
    bool restore_patch() {
        if(!state_) return true;
        return state_->patch.restore();
    }
    // Restoring a CALL does not drain threads already in its callback.
    // The host must prevent new native CALL producers first. Drain captured
    // callback leases here, outside any thread-freeze interval, before freeing.
    bool release_after_quiescence() {
        if(!state_) return true;
        if(state_->patch.active()) return false;
        if(!state_->retired) {
            if(!publication_.withdraw(state_.get())) return false;
            state_->retired=true;
        }
        if(!state_->patch.release_after_quiescence()) return false;
        state_.reset();return true;
    }
    bool restore() {return restore_patch() && release_after_quiescence();}
};
}
