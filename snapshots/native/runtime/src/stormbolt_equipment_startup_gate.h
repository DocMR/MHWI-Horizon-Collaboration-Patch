#pragma once
#include "hooks.h"
#include <atomic>
#include <cstdint>
namespace stormslinger {
// Process-lifetime native entry gate. This serializes only startup attempts,
// not every native producer. It does NOT certify shooting object borrow.
class StormboltEquipmentStartupGate {
public:
    using Native=bool(*)(void*,unsigned);
    using Start=int(*)(void*);
private:
    HookTransaction root_;
    Native original_=nullptr;
    Start start_=nullptr;
    void* context_=nullptr;
    void** singleton_=nullptr;
    std::uintptr_t vtable_=0;
    SRWLOCK attempt_=SRWLOCK_INIT;
    std::atomic<bool> armed_{false};
    std::atomic<int> result_{-13};
    // Last completed/failed installation operation, preserved across cleanup.
    // 0 idle,1 precheck,2 library begin,3 hook create/queue,4 owner publish,
    // 5 shared commit,6 armed. MH status is meaningful for stages2..5.
    std::atomic<int> install_stage_{0},install_status_{MH_OK};
    std::atomic<DWORD> failure_origin_{0},failure_error_{0};
    void capture_failure() {
        DWORD origin=0,error=0;MH_GetFailureDetail(&origin,&error);
        failure_origin_.store(origin);failure_error_.store(error);
    }
    inline static std::atomic<StormboltEquipmentStartupGate*> published_{nullptr};
    static bool invoke(void* manager,unsigned mode) {
        auto* self=published_.load(std::memory_order_acquire);
        // Publication precedes hook enable and stays alive for process lifetime.
        if(self->armed_.load(std::memory_order_acquire)) self->attempt(manager);
        return self->original_(manager,mode);
    }
    void attempt(void* manager) {
        AcquireSRWLockExclusive(&attempt_);
        struct Unlock {SRWLOCK* lock;~Unlock(){ReleaseSRWLockExclusive(lock);}} unlock{&attempt_};
        if(result_.load()!=-13) return;
        void* current=nullptr;std::uintptr_t table=0;int phase=-1;SIZE_T count=0;
        if(!ReadProcessMemory(GetCurrentProcess(),singleton_,&current,sizeof(current),&count) ||
            count!=sizeof(current) || current!=manager || !manager) return;
        if(!ReadProcessMemory(GetCurrentProcess(),manager,&table,sizeof(table),&count) ||
            count!=sizeof(table) || table!=vtable_) return;
        if(!ReadProcessMemory(GetCurrentProcess(),static_cast<unsigned char*>(manager)+0x38,
            &phase,sizeof(phase),&count) || count!=sizeof(phase)) {result_.store(-19);return;}
        if(phase<0 || phase>=4) {result_.store(-19);return;}
        try {result_.store(start_(context_));}
        catch(const std::bad_alloc&) {result_.store(-23);}
    }
public:
    StormboltEquipmentStartupGate()=default;
    StormboltEquipmentStartupGate(const StormboltEquipmentStartupGate&)=delete;
    StormboltEquipmentStartupGate& operator=(const StormboltEquipmentStartupGate&)=delete;
    HookTransaction& transaction() noexcept {return root_;}
    int result() const noexcept {return result_.load();}
    DWORD failure_origin() const noexcept {return failure_origin_.load();}
    DWORD failure_error() const noexcept {return failure_error_.load();}
    int install_stage() const noexcept {return install_stage_.load();}
    int install_status() const noexcept {return install_status_.load();}
    bool armed() const noexcept {return armed_.load(std::memory_order_acquire);}
    // Timeout closes pending startup work, retaining the process-lifetime detour.
    // A concurrent successful attempt wins; its result must not be overwritten.
    int cancel_pending(int code) {
        AcquireSRWLockExclusive(&attempt_);
        if(result_.load()==-13) result_.store(code);
        const int result=result_.load();
        ReleaseSRWLockExclusive(&attempt_);return result;
    }
    // Caller authenticates the target before arm; lifetime is permanent once
    // enabled. Do not stop root from inside this callback's native trampoline.
    MH_STATUS arm(Native target,void** singleton,std::uintptr_t vtable,Start start,void* context) {
        if(armed()) return MH_OK;
        if(!target || !singleton || !vtable || !start || root_.has_library_state()) return MH_ERROR_NOT_INITIALIZED;
        // A manager already past the window may never call this dispatcher
        // again. Reject now instead of installing a dormant detour and waiting
        // for the worker timeout. The entry checks still handle an install race.
        install_stage_.store(1);
        void* current=nullptr;std::uintptr_t table=0;int phase=-1;SIZE_T count=0;
        bool readable=ReadProcessMemory(GetCurrentProcess(),singleton,&current,sizeof(current),&count) && count==sizeof(current);
        if(readable && current) {
            readable=ReadProcessMemory(GetCurrentProcess(),current,&table,sizeof(table),&count) && count==sizeof(table) && table==vtable;
            if(readable) readable=ReadProcessMemory(GetCurrentProcess(),static_cast<unsigned char*>(current)+0x38,
                &phase,sizeof(phase),&count) && count==sizeof(phase) && phase>=0 && phase<4;
        }
        if(!readable) {result_.store(-19);return MH_ERROR_UNSUPPORTED_FUNCTION;}
        MH_ResetFailureDetail();
        install_stage_.store(2);
        auto status=root_.begin();install_status_.store(status);if(status!=MH_OK) {capture_failure();return status;}
        singleton_=singleton;vtable_=vtable;start_=start;context_=context;
        install_stage_.store(3);
        status=root_.add(reinterpret_cast<void*>(target),reinterpret_cast<void*>(&invoke),
            reinterpret_cast<void**>(&original_));
        install_status_.store(status);if(status!=MH_OK) {capture_failure();return status;}
        install_stage_.store(4);
        StormboltEquipmentStartupGate* empty=nullptr;
        if(!published_.compare_exchange_strong(empty,this,std::memory_order_release)) {
            install_status_.store(MH_ERROR_ALREADY_CREATED);root_.stop();return MH_ERROR_ALREADY_CREATED;
        }
        // On commit failure retain owner/trampoline: a native call may already
        // have selected this detour. No automatic destruction or retry re-arm.
        install_stage_.store(5);
        status=root_.commit_pending_without_rollback();install_status_.store(status);
        if(status!=MH_OK) capture_failure();
        if(status==MH_OK) {armed_.store(true,std::memory_order_release);install_stage_.store(6);}
        return status;
    }
};
} // namespace stormslinger
