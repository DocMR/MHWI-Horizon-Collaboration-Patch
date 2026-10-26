#pragma once
#include "generated_source_graph_plan.h"
#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace stormslinger {
struct StormboltGraphList {
    StormboltGraphLink* links=nullptr;
    std::uint32_t count=0,reserved=0;
};
struct StormboltGraphState {StormboltGraphList lists[2];};
static_assert(sizeof(StormboltGraphList)==16 && sizeof(StormboltGraphState)==32);

// Borrow rows and condition registry at a quiescent point after native build.
// Caller must prevent native destruction/rebuild until restore() succeeds.
// The separate ShootGatling condition must already exist in an extended registry.
class StormboltSourceGraphPatch {
    struct Entry {
        StormboltGraphList* slot=nullptr;
        StormboltGraphLink* original=nullptr;
        std::uint32_t original_count=0;
        std::unique_ptr<StormboltGraphLink[]> owned;
        std::uint32_t count=0;
    };
    std::vector<Entry> entries_;
    bool active_=false;
public:
    StormboltSourceGraphPatch()=default;
    StormboltSourceGraphPatch(const StormboltSourceGraphPatch&)=delete;
    StormboltSourceGraphPatch& operator=(const StormboltSourceGraphPatch&)=delete;
    bool active() const noexcept {return active_;}
    bool prepare(StormboltGraphState* rows,std::size_t count,
        void* const* conditions,std::size_t condition_count,std::uint32_t shoot_key) {
        if(active_) return false;
        entries_.clear();
        if(!rows || count!=120 || !conditions || shoot_key<798 || shoot_key>0x7FFFFFFFu ||
            shoot_key>=condition_count || !conditions[776] || !conditions[shoot_key]) return false;
        std::vector<Entry> staged;
        staged.reserve(std::size(source_graph_edits));
        for(const auto& edit:source_graph_edits) {
            auto& slot=rows[edit.state].lists[edit.secondary];
            if(slot.count!=edit.count || !slot.links ||
                std::memcmp(slot.links,edit.expected,edit.count*sizeof(StormboltGraphLink))) return false;
            Entry entry;
            entry.slot=&slot;entry.original=slot.links;entry.original_count=slot.count;
            entry.count=static_cast<std::uint32_t>(edit.count+edit.added);
            entry.owned=std::make_unique<StormboltGraphLink[]>(entry.count);
            std::memcpy(entry.owned.get(),slot.links,edit.index*sizeof(StormboltGraphLink));
            std::memcpy(entry.owned.get()+edit.index,edit.inserted,edit.added*sizeof(StormboltGraphLink));
            for(std::size_t n=0;n<edit.added;++n) {
                auto& link=entry.owned[edit.index+n];
                if(link.group==1 && link.key==-30) link.key=static_cast<std::int32_t>(shoot_key);
            }
            std::memcpy(entry.owned.get()+edit.index+edit.added,slot.links+edit.index,
                (edit.count-edit.index)*sizeof(StormboltGraphLink));
            staged.push_back(std::move(entry));
        }
        entries_=std::move(staged);
        return true;
    }
    bool activate() noexcept {
        if(active_ || entries_.size()!=std::size(source_graph_edits)) return false;
        for(const auto& entry:entries_) {
            if(entry.slot->links!=entry.original || entry.slot->count!=entry.original_count) return false;
        }
        for(auto& entry:entries_) {
            entry.slot->links=entry.owned.get();entry.slot->count=entry.count;
        }
        active_=true;return true;
    }
    bool restore() noexcept {
        if(!active_) {entries_.clear();return true;}
        for(const auto& entry:entries_) {
            if(entry.slot->links!=entry.owned.get() || entry.slot->count!=entry.count) return false;
        }
        for(auto& entry:entries_) {
            entry.slot->links=entry.original;entry.slot->count=entry.original_count;
        }
        active_=false;entries_.clear();return true;
    }
    void retain_published() noexcept {
        for(auto& entry:entries_) entry.owned.release();
        entries_.clear();active_=false;
    }
    ~StormboltSourceGraphPatch() {
        if(!restore()) for(auto& entry:entries_) entry.owned.release();
    }
};
} // namespace stormslinger
