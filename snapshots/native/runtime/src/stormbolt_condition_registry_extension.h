#pragma once
#include <cstdint>
#include <cstring>
#include <memory>

namespace stormslinger {
// Native machine group1 array is at +D0, count at +D8. Its adjacent reserved
// DWORD is not capacity and must remain untouched. Caller borrows live slots.
class StormboltConditionRegistryExtension {
    void*** slot_=nullptr;
    std::uint32_t* count_slot_=nullptr;
    void** original_=nullptr;
    std::unique_ptr<void*[]> extended_;
    bool active_=false;
public:
    static constexpr std::uint32_t native_count=798,new_key=798;
    StormboltConditionRegistryExtension()=default;
    StormboltConditionRegistryExtension(const StormboltConditionRegistryExtension&)=delete;
    StormboltConditionRegistryExtension& operator=(const StormboltConditionRegistryExtension&)=delete;
    bool active() const noexcept {return active_;}
    bool prepare(void*** slot,std::uint32_t* count,void* condition) {
        if(active_) return false;
        extended_.reset();slot_=nullptr;count_slot_=nullptr;original_=nullptr;
        if(!slot || !count || !*slot || *count!=native_count || !condition) return false;
        auto candidate=std::make_unique<void*[]>(native_count+1);
        std::memcpy(candidate.get(),*slot,native_count*sizeof(void*));
        candidate[new_key]=condition;
        original_=*slot;slot_=slot;count_slot_=count;extended_=std::move(candidate);
        return true;
    }
    bool activate() noexcept {
        if(active_ || !extended_ || *slot_!=original_ || *count_slot_!=native_count) return false;
        *slot_=extended_.get();*count_slot_=native_count+1;active_=true;return true;
    }
    void* const* prepared_objects() const noexcept {return extended_.get();}
    bool restore() noexcept {
        if(active_) {
            if(*slot_!=extended_.get() || *count_slot_!=native_count+1) return false;
            *slot_=original_;*count_slot_=native_count;active_=false;
        }
        extended_.reset();slot_=nullptr;count_slot_=nullptr;original_=nullptr;return true;
    }
    // A graph still publishing the new key forbids implicit registry rollback.
    void retain_published() noexcept {
        extended_.release();active_=false;slot_=nullptr;count_slot_=nullptr;original_=nullptr;
    }
    ~StormboltConditionRegistryExtension() {if(!restore()) extended_.release();}
};
} // namespace stormslinger
