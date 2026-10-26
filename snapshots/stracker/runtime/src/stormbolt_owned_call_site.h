#pragma once
#include "hooks.h"
#include "stormbolt_loop_call_patch.h"
namespace stormslinger {
// Exact five-byte CALL publication through the existing MinHook library. Unlike
// a JMP hook, a thread already at its relay has a real pushed return address.
// Native producer quiescence and callback owner drain remain host obligations.
template<std::size_t Span>
class StormboltOwnedReplayCallSite {
    static_assert(Span>=5 && Span<=12);
    HookTransaction local_;
    HookTransaction* transaction_;
    void* site_=nullptr;
    std::array<unsigned char,Span> before_{},after_{};
    DWORD protection_=0;
    bool registered_=false,published_=false;
    bool maintain() {
        // MinHook's stock writer does not report final flush/protection errors.
        // Keep this CALL owner until both original publication invariants hold.
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site_,before_.size())!=FALSE;
        MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(site_,&region,sizeof(region))) return false;
        if(region.Protect!=protection_) {
            DWORD unused=0;
            if(!VirtualProtect(site_,before_.size(),protection_,&unused)) return false;
        }
        return flushed;
    }
public:
    explicit StormboltOwnedReplayCallSite(HookTransaction* shared=nullptr)
        :transaction_(shared?shared:&local_) {}
    StormboltOwnedReplayCallSite(const StormboltOwnedReplayCallSite&)=delete;
    bool active() const noexcept {return published_;}
    bool prepare(void* site,std::uintptr_t entry,const std::array<unsigned char,Span>& expected) {
        if(registered_ || !site || !entry || (Span==5 && expected[0]!=0xE8)) return false;
        std::array<unsigned char,Span> live{};SIZE_T count=0;
        if(!ReadProcessMemory(GetCurrentProcess(),site,live.data(),live.size(),&count) || count!=live.size() || live!=expected) return false;
        MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(site,&region,sizeof(region)) || region.State!=MEM_COMMIT) return false;
        const auto offset=reinterpret_cast<std::uintptr_t>(site)-reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if(offset>region.RegionSize || expected.size()>region.RegionSize-offset) return false;
        protection_=region.Protect;
        if(transaction_==&local_) {
            if(local_.begin()!=MH_OK) return false;
            local_.retain_failed_adds_for_coordinator();
        }
        site_=site;before_=expected;
        void* relay=nullptr;
        const auto added=Span==5 ? transaction_->add_call(site,reinterpret_cast<void*>(entry),&relay) :
            transaction_->add_replay_call(site,reinterpret_cast<void*>(entry),&relay,static_cast<UINT>(Span));
        if(!relay) return false;
        registered_=true;published_=true;
        std::array<unsigned char,5> call{};
        if(!build_stormbolt_loop_call(reinterpret_cast<std::uintptr_t>(site),reinterpret_cast<std::uintptr_t>(relay),call)) return false;
        after_=before_;std::copy(call.begin(),call.end(),after_.begin());
        return added==MH_OK;
    }
    bool activate() {
        if(!registered_ || !published_) return false;
        if(transaction_==&local_ && local_.commit_pending_without_rollback()!=MH_OK) return false;
        // Shared owner committed ALL modules once before activation; never
        // apply its global queue from a component's activation method.
        if(!transaction_->last.active) return false;
        std::array<unsigned char,Span> live{};SIZE_T count=0;
        if(!ReadProcessMemory(GetCurrentProcess(),site_,live.data(),live.size(),&count) || count!=live.size() || live!=after_) return false;
        return maintain();
    }
    bool restore() {
        if(!registered_ || !published_) return true;
        std::array<unsigned char,Span> live{};SIZE_T count=0;
        if(!ReadProcessMemory(GetCurrentProcess(),site_,live.data(),live.size(),&count) || count!=live.size() ||
            (live!=before_ && live!=after_)) return false;
        if(MH_QueueDisableHook(site_)!=MH_OK) return false;
        const auto status=MH_DisableHook(site_);
        if(status!=MH_OK && status!=MH_ERROR_DISABLED) return false;
        if(!maintain()) return false;
        published_=false;return true;
    }
    // Called only after publication closes and captured callback leases drain.
    // A shared owner performs stop itself, before releasing our dependencies.
    bool release_after_quiescence() {
        if(published_) return false;
        if(transaction_==&local_ && local_.stop()!=MH_OK) return false;
        if(transaction_->has_library_state()) return false;
        registered_=false;return true;
    }
};
using StormboltOwnedCallSite=StormboltOwnedReplayCallSite<5>;
} // namespace stormslinger
