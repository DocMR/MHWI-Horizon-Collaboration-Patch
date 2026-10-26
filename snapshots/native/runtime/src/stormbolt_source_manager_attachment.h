#pragma once
#include "stormbolt_shoot_gatling_condition.h"
#include "stormbolt_source_graph_session.h"
#include "stormbolt_live_probe.h"

namespace stormslinger {
struct StormboltSourceManagerBindings {
    std::uintptr_t manager_vtable=0;
    StormboltShootConditionBindings condition{};
};
enum class StormboltManagerAttachResult {attached,unrelated,not_ready,rejected};

// The native caller borrows a live manager and serializes these calls. Restore
// before118DDC0 rebuild or118E810 destruction; attach after native tables exist.
// Runtime detours are separate from this ownership/lifecycle component.
template<std::size_t WeaponCapacity>
class StormboltSourceManagerAttachment {
    using Condition=StormboltShootGatlingCondition<WeaponCapacity>;
    StormboltInstanceStore<WeaponCapacity>& store_;
    std::unique_ptr<Condition> condition_;
    StormboltSourceGraphSession graph_;
    void* manager_=nullptr;
    StormboltInstanceToken weapon_token_{};
public:
    explicit StormboltSourceManagerAttachment(StormboltInstanceStore<WeaponCapacity>& store):store_(store) {}
    bool active() const noexcept {return graph_.active();}
    void* manager() const noexcept {return manager_;}
    void* condition() const noexcept {return condition_?condition_->object():nullptr;}
    bool matches_current_weapon() {
        if(!manager_ || !active()) return false;
        auto* actor=stormbolt_source_field<void*>(manager_,0xD90);
        if(!actor) return false;
        auto* weapon=stormbolt_source_field<void*>(actor,0x76B0);
        StormboltInstanceToken current{};
        return store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),current) &&
            current.address==weapon_token_.address && current.generation==weapon_token_.generation;
    }
    bool before_rebuild_or_destroy() {
        if(!graph_.restore()) return false;
        if(condition_) condition_->destroy_after_detach();
        condition_.reset();manager_=nullptr;weapon_token_={};return true;
    }
    StormboltManagerAttachResult after_rebuild(void* manager,const StormboltSourceManagerBindings& api) {
        if(active() || condition_ || !manager || !api.manager_vtable ||
            stormbolt_source_field<std::uintptr_t>(manager,0)!=api.manager_vtable)
            return StormboltManagerAttachResult::rejected;
        auto* actor=stormbolt_source_field<void*>(manager,0xD90);
        if(!actor || stormbolt_source_field<std::uintptr_t>(actor,0)!=api.condition.actor_vtable ||
            !(stormbolt_source_field<std::uint8_t>(actor,0xC)&0xE) ||
            stormbolt_source_field<void*>(actor,0x12608)!=manager)
            return StormboltManagerAttachResult::rejected;
        auto* weapon=stormbolt_source_field<void*>(actor,0x76B0);
        if(!weapon) return StormboltManagerAttachResult::not_ready;
        StormboltInstanceToken token{};
        if(!store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token))
            return StormboltManagerAttachResult::not_ready;
        if(stormbolt_source_field<std::uintptr_t>(weapon,0)!=api.condition.weapon_vtable ||
            !(stormbolt_source_field<std::uint8_t>(weapon,0xC)&0xE) ||
            stormbolt_source_field<void*>(weapon,0x9B0)!=actor)
            return StormboltManagerAttachResult::rejected;
        const auto id=stormbolt_source_field<std::uint32_t>(weapon,0x9FC);
        if(stormbolt_source_field<std::uint32_t>(weapon,0x9F8)!=13 || (id!=238 && id!=239))
            return StormboltManagerAttachResult::unrelated;
        stormbolt_live_probe.graph_attempts.fetch_add(1,std::memory_order_relaxed);
        stormbolt_live_probe.graph_stage.store(1,std::memory_order_relaxed);
        auto* rows=stormbolt_source_field<StormboltGraphState*>(manager,0xAB0);
        const auto count=stormbolt_source_field<std::uint32_t>(manager,0xAB8);
        stormbolt_live_probe.graph_rows.store(count,std::memory_order_relaxed);
        stormbolt_live_probe.graph_registry_count.store(
            stormbolt_source_field<std::uint32_t>(manager,0xAD8),std::memory_order_relaxed);
        if(!rows || count!=120) return StormboltManagerAttachResult::not_ready;
        stormbolt_live_probe.graph_stage.store(2,std::memory_order_relaxed);
        auto condition=std::make_unique<Condition>(store_);
        if(!condition->create(actor,api.condition)) return StormboltManagerAttachResult::not_ready;
        stormbolt_live_probe.graph_stage.store(3,std::memory_order_relaxed);
        auto*** registry=reinterpret_cast<void***>(static_cast<unsigned char*>(manager)+0xAD0);
        auto* registry_count=reinterpret_cast<std::uint32_t*>(static_cast<unsigned char*>(manager)+0xAD8);
        if(!graph_.install(rows,count,registry,registry_count,condition->object())) {
            if(graph_.active()) {condition_=std::move(condition);manager_=manager;weapon_token_=token;}
            return StormboltManagerAttachResult::rejected;
        }
        condition_=std::move(condition);manager_=manager;weapon_token_=token;
        stormbolt_live_probe.graph_stage.store(4,std::memory_order_relaxed);
        return StormboltManagerAttachResult::attached;
    }
    ~StormboltSourceManagerAttachment() {
        // Keep callback owner/vtable alive with any unrestored published graph.
        if(!before_rebuild_or_destroy()) condition_.release();
    }
};
} // namespace stormslinger
