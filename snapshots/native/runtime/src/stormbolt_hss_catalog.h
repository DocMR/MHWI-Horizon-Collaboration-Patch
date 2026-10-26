#pragma once
#include "hooks.h"
#include "stormbolt_native_memory.h"
#include <atomic>
#include <cstring>
#include <vector>

namespace stormslinger {
// sMhGUI category3: pointer14FE0/count14FE8, rows {DTI,path,flags}.
class StormboltHssCatalog {
    struct Row {void* type;const char* path;std::uint64_t flags;};
    static_assert(sizeof(Row)==24);
    using Preload=void(*)(void*,unsigned,unsigned);
    using Create=void(*)(void*,unsigned,void*);
    using Remove=void(*)(void*,unsigned);
    Preload preload_{};Create create_{};Remove remove_{};
    void* type_{};
    bool enabled_{};
    std::atomic<unsigned> failures_{};
    inline static StormboltHssCatalog* active_{};
    class Scope {
        StormboltHssCatalog& owner_;
        unsigned char* manager_{};
        Row* before_{};std::uint32_t count_{};
        std::vector<Row> rows_;
    public:
        Scope(StormboltHssCatalog& owner,void* manager,unsigned category):owner_(owner) {
            if(category!=3 || !owner_.enabled_) return;
            const auto address=reinterpret_cast<std::uintptr_t>(manager);
            if(!stormbolt_native_span(address,0x14FE0,12,true) ||
                !read_stormbolt_field(address,0x14FE0,before_) ||
                !read_stormbolt_field(address,0x14FE8,count_) || !before_ || count_==0xFFFFFFFFu) {
                ++owner_.failures_;return;
            }
            try {
                rows_.resize(std::size_t(count_)+1);
                SIZE_T bytes{};
                if(count_ && (!ReadProcessMemory(GetCurrentProcess(),before_,rows_.data(),
                    std::size_t(count_)*sizeof(Row),&bytes) || bytes!=std::size_t(count_)*sizeof(Row))) {
                    ++owner_.failures_;return;
                }
                for(unsigned i=0;i<count_;++i)if(rows_[i].type==owner_.type_)return;
                rows_[count_]={owner_.type_,"ui\\hud\\hud_hss",0};
                auto* pointer=rows_.data();const auto size=count_+1;
                manager_=static_cast<unsigned char*>(manager);
                std::memcpy(manager_+0x14FE0,&pointer,sizeof(pointer));
                std::memcpy(manager_+0x14FE8,&size,sizeof(size));
            } catch(const std::bad_alloc&) {++owner_.failures_;}
        }
        ~Scope() {
            if(!manager_)return;
            Row* current{};std::uint32_t count{};
            std::memcpy(&current,manager_+0x14FE0,sizeof(current));
            std::memcpy(&count,manager_+0x14FE8,sizeof(count));
            if(current!=rows_.data()) {++owner_.failures_;return;}
            if(count!=count_+1)++owner_.failures_;
            std::memcpy(manager_+0x14FE0,&before_,sizeof(before_));
            std::memcpy(manager_+0x14FE8,&count_,sizeof(count_));
        }
    };
    static void preload(void* manager,unsigned resource_group,unsigned category) {
        auto& self=*active_;Scope scope(self,manager,category);
        self.preload_(manager,resource_group,category);
    }
    static void create(void* manager,unsigned category,void* output) {
        auto& self=*active_;Scope scope(self,manager,category);
        self.create_(manager,category,output);
    }
    static void remove(void* manager,unsigned category) {
        auto& self=*active_;Scope scope(self,manager,category);
        self.remove_(manager,category);
    }
public:
    MH_STATUS prepare(HookTransaction& transaction,std::uintptr_t image,void* type) {
        if(!type || (active_ && active_!=this))return MH_ERROR_NOT_INITIALIZED;
        type_=type;active_=this;enabled_=true;
        auto status=transaction.add(reinterpret_cast<void*>(image+0x1AD8D20),
            reinterpret_cast<void*>(&preload),reinterpret_cast<void**>(&preload_));
        if(status!=MH_OK)return status;
        status=transaction.add(reinterpret_cast<void*>(image+0x1AD6230),
            reinterpret_cast<void*>(&create),reinterpret_cast<void**>(&create_));
        if(status!=MH_OK)return status;
        return transaction.add(reinterpret_cast<void*>(image+0x1AE00F0),
            reinterpret_cast<void*>(&remove),reinterpret_cast<void**>(&remove_));
    }
    unsigned failures() const noexcept{return failures_.load();}
    // Caller establishes the same native quiescence as transaction restoration.
    void withdraw() noexcept {enabled_=false;}
};
} // namespace stormslinger
