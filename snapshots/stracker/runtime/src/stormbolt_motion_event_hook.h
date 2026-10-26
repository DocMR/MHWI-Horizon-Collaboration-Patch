#pragma once
#include "stormbolt_motion_events.h"
#include "stormbolt_shell_hook.h"
#include "hooks.h"

namespace stormslinger {
using StormboltTimelineBind=void(*)(void*,const void*,void*,float,bool);
struct StormboltMotionEventBindings {
    StormboltTimelineBind entry{}; // 24A7F50, including float and fifth bool.
    std::uintptr_t actor_vtable{},weapon_vtable{};
    bool (*dispatch_ready)(void*){};
    void* dispatch_context{};
};

template<std::size_t Capacity>
class StormboltMotionEventHook {
    StormboltInstanceStore<Capacity>& store_;
    StormboltMotionEventBindings bindings_{};
    StormboltTimelineBind original_{};
    bool enabled_=false;
    inline static StormboltMotionEventHook* owner_{};
    // Native players retain pointers into the selected tree after the callback.
    // Session teardown must not free it. The DLL remains loaded until exit.
    static const StormboltMotionEvents& events() {
        static const StormboltMotionEvents data;
        return data;
    }
    bool select(void* player,void* parameters,const void*& root) {
        const auto timeline=reinterpret_cast<std::uintptr_t>(player);
        const auto list=reinterpret_cast<std::uintptr_t>(parameters);
        const void* selected{};
        if(!enabled_ || !timeline || !list ||
            (bindings_.dispatch_ready && !bindings_.dispatch_ready(bindings_.dispatch_context)) ||
            !root) return false;
        // Native registration uses actor+6B8+channel*40. Its layer matches
        // component80+channel*2570; four variants each own a timeline at+148.
        for(std::uintptr_t channel=0;channel<6;++channel) {
            const auto displacement=0x6B8+channel*0x40;
            if(list<displacement) continue;
            const auto actor=list-displacement;
            std::uintptr_t table{},component{},weapon{},native_owner{},resource{},bound_resource{};
            std::uint8_t alive{};
            struct Identity {std::uint32_t category,id;} identity{};
            if(!read_stormbolt_field(actor,0,table) || table!=bindings_.actor_vtable ||
                !read_stormbolt_field(actor,0xC,alive) || !(alive&0xE) ||
                !read_stormbolt_field(actor,0x468,component) || !component ||
                !read_stormbolt_field(actor,0x76B0,weapon) ||
                !read_stormbolt_field(weapon,0,table) || table!=bindings_.weapon_vtable ||
                !read_stormbolt_field(weapon,0xC,alive) || !(alive&0xE) ||
                !read_stormbolt_field(weapon,0x9B0,native_owner) || native_owner!=actor ||
                !read_stormbolt_field(weapon,0x9F8,identity) || identity.category!=13 ||
                (identity.id!=238 && identity.id!=239)) continue;
            bool owned_player=false;
            for(std::uintptr_t variant=0;variant<4;++variant)
                if(timeline==component+0x80+channel*0x2570+0x30+variant*0x1B0+0x148)
                    owned_player=true;
            // The weapon motion player is not 24A7D80's generic wrapper.
            // Live player-8 is rMotion; its+A8 is the LMT header. Native
            // 22AC150 reads WORD count+6 and inline motion pointers at+10.
            // Each 96-byte motion header owns its event root at+58.
            if(!owned_player || timeline<8 ||
                !read_stormbolt_field(component,0xE120+12*8,resource) || !resource ||
                !read_stormbolt_field(timeline-8,0,bound_resource) || bound_resource!=resource) continue;
            std::uintptr_t lmt{};
            std::uint16_t count{};
            if(!read_stormbolt_field(resource,0xA8,lmt) || !lmt ||
                !read_stormbolt_field(lmt,6,count)) continue;
            bool matched=false;
            for(const auto& definition:stormbolt_motion_event_roots) {
                const void* original_root{};
                std::uintptr_t motion{};
                if(definition.slot<count &&
                    read_stormbolt_field(lmt,0x10+definition.slot*sizeof(void*),motion) && motion &&
                    read_stormbolt_field(motion,0x58,original_root) &&
                    original_root==root && events().select(definition.slot,selected)) {
                    matched=true;break;
                }
            }
            if(!matched) continue;
            StormboltInstanceToken token{};
            std::uint32_t part_count{};
            if(!store_.token_for_live_address(weapon,token) ||
                !read_stormbolt_field(weapon,0x2388,part_count) || !part_count) continue;
            root=selected; // nullptr is an intentional empty source track.
            return true;
        }
        return false;
    }
    static void callback(void* player,const void* root,void* parameters,float frame,bool reset) {
        auto& self=*owner_;
        self.select(player,parameters,root);
        self.original_(player,root,parameters,frame,reset);
    }
public:
    explicit StormboltMotionEventHook(StormboltInstanceStore<Capacity>& store):store_(store) {}
    template<class Transaction>
    MH_STATUS prepare(Transaction& transaction,const StormboltMotionEventBindings& bindings) {
        if(owner_ || !bindings.entry || !bindings.actor_vtable || !bindings.weapon_vtable)
            return MH_ERROR_NOT_INITIALIZED;
        (void)events();bindings_=bindings;owner_=this;enabled_=true;
        return transaction.add(reinterpret_cast<void*>(bindings.entry),reinterpret_cast<void*>(&callback),
            reinterpret_cast<void**>(&original_));
    }
    void withdraw_while_quiescent() noexcept {enabled_=false;}
    // Only after the shared transaction has stopped all native callbacks.
    void release_after_stop() noexcept {if(owner_==this) owner_=nullptr;}
};
} // namespace stormslinger
