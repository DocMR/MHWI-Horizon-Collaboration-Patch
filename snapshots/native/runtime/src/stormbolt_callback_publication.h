#pragma once
#include <windows.h>

namespace stormslinger {
// Protects C++ callback owners only. Hosts must separately stop native producers
// and synchronize instruction writes. Never drain while worker threads are frozen.
template<class Owner>
class StormboltCallbackPublication {
    SRWLOCK lock_=SRWLOCK_INIT;
    CONDITION_VARIABLE drained_=CONDITION_VARIABLE_INIT;
    Owner* owner_=nullptr;
    unsigned calls_=0;
    bool accepting_=false;
    void release() {
        AcquireSRWLockExclusive(&lock_);
        if(--calls_==0) WakeAllConditionVariable(&drained_);
        ReleaseSRWLockExclusive(&lock_);
    }
public:
    StormboltCallbackPublication()=default;
    StormboltCallbackPublication(const StormboltCallbackPublication&)=delete;
    StormboltCallbackPublication& operator=(const StormboltCallbackPublication&)=delete;
    struct Lease {
        StormboltCallbackPublication* publication;
        Owner* owner;
        Lease(StormboltCallbackPublication* p,Owner* o):publication(p),owner(o) {}
        Lease(const Lease&)=delete;
        Lease& operator=(const Lease&)=delete;
        ~Lease() {if(owner) publication->release();}
    };
    Lease acquire() {
        AcquireSRWLockExclusive(&lock_);
        auto* value=accepting_?owner_:nullptr;
        if(value) ++calls_;
        ReleaseSRWLockExclusive(&lock_);
        return Lease(this,value);
    }
    bool owns(Owner* value) {
        AcquireSRWLockExclusive(&lock_);
        const bool result=owner_==value;
        ReleaseSRWLockExclusive(&lock_);
        return result;
    }
    bool publish(Owner* value) {
        if(!value) return false;
        AcquireSRWLockExclusive(&lock_);
        const bool result=!owner_;
        if(result) {owner_=value;accepting_=true;}
        ReleaseSRWLockExclusive(&lock_);
        return result;
    }
    bool withdraw(Owner* value,bool (*retired)(Owner*)=nullptr) {
        AcquireSRWLockExclusive(&lock_);
        if(!value || owner_!=value) {ReleaseSRWLockExclusive(&lock_);return false;}
        accepting_=false;
        while(calls_) {
            if(!SleepConditionVariableSRW(&drained_,&lock_,INFINITE,0)) {
                ReleaseSRWLockExclusive(&lock_);return false;
            }
        }
        // Cross-CALL protocols can retain a closed owner until their native
        // completion arrives. A retry must not admit new producer callbacks.
        if(retired && !retired(value)) {ReleaseSRWLockExclusive(&lock_);return false;}
        owner_=nullptr;
        ReleaseSRWLockExclusive(&lock_);
        return true;
    }
};
}
