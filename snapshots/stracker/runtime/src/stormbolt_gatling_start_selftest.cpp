#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_gatling_start_update.h"
#include "stormbolt_gatling_start_entry.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes = std::array<std::byte,N>;
Bytes<0x30> g_action;
Bytes<0x9000> g_actor, g_other;
Bytes<0x2860> g_weapon;
Bytes<0x200> g_event;
int g_component;
bool g_swap_flag = false, g_swap_effect = false, g_throw = false, g_result = false;
void* g_queried = nullptr;
std::vector<unsigned> g_calls;
template<class T> void put(void* p, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(p)+offset,&value,sizeof(value));
}
void flag(void* actor, std::uint32_t value) {
    assert(actor == g_actor.data()); g_calls.push_back(value);
    if(g_swap_flag) put(g_action.data(),0x28,g_other.data());
}
void* event(void* actor) { assert(actor==g_actor.data()); return g_event.data(); }
void effect(void* component, std::uint32_t group, std::uint32_t record, std::uint32_t parameter) {
    assert(component==&g_component && group==0x7D0 && record==0 && parameter==0xFFFFFFFF);
    g_calls.push_back(0x7D0);
    if(g_throw) throw std::runtime_error("fixture native effect");
    if(g_swap_effect) put(g_action.data(),0x28,g_other.data());
}
bool query(void* actor, std::uint32_t channel) {
    assert(channel==0); g_queried=actor; g_calls.push_back(0); return g_result;
}
bool g_local = false, g_motion_swap = false;
unsigned g_motions = 0, g_item_reads = 0;
int g_manager=0;
void* g_manager_slot=&g_manager;
const std::array<std::uint32_t,41> g_ammo_items{137,138,139,140,141,142,143,144,145,149,150,151,167,146,147,148,168,152,153,155,154,156,157,158,159,160,161,162,163,164,165,166,170,171,0,0,169,137,137,137,137};
void* local_player(void* manager) {
    assert(manager==&g_manager); ++g_item_reads;
    return g_local ? g_actor.data() : g_other.data();
}
bool motion(void* actor, std::uint32_t packed, std::uint32_t id, std::uint32_t mode,
    std::uint32_t extra, float start, float rate) {
    const auto layer=g_motions++;
    assert(layer<3 && packed==0x200+layer && id==0xC0FF-layer);
    assert(mode==(layer==0 ? 2u : layer==1 ? 1u : 3u));
    assert(extra==0 && start==-1.0f && rate==2.0f);
    assert(actor==(g_motion_swap && layer>0 ? g_other.data() : g_actor.data()));
    if(g_motion_swap) put(g_action.data(),0x28,g_other.data());
    return false; // Native gate rejection still permits following layers.
}
}
int main() {
    StormboltInstanceStore<1> store;
    StormboltSpecialToNormalApi api{};
    api.actor_flag=flag; api.event_object=event; api.weapon_effect=effect; api.query=query;
    StormboltGatlingStartUpdate<1> update(store,api);
    put(g_actor.data(),0xC,std::uint8_t{2}); put(g_other.data(),0xC,std::uint8_t{2});
    put(g_actor.data(),0x76B0,g_weapon.data()); put(g_actor.data(),0x8808,&g_component);
    put(g_weapon.data(),0xC,std::uint8_t{2}); put(g_weapon.data(),0x9B0,g_actor.data());
    put(g_weapon.data(),0x9F8,std::uint32_t{13});
    const auto token=store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data()));
    unsigned cases=0;
    for (std::uint32_t id : {238u,239u}) for (bool trigger : {false,true})
    for (bool result : {false,true}) for (bool swap_flag : {false,true}) for (bool swap_effect : {false,true}) {
        g_calls.clear(); g_queried=nullptr; g_result=result; g_swap_flag=swap_flag; g_swap_effect=swap_effect;
        put(g_action.data(),0x28,g_actor.data()); put(g_weapon.data(),0x9FC,id);
        put(g_event.data(),0x190,std::uint32_t{trigger ? 0xA7u : 0xA5u});
        const auto before=g_weapon;
        const auto outcome=update.update(g_action.data());
        assert(outcome.status==StormboltSpecialActionStatus::processed && outcome.animation_complete==result);
        assert(g_weapon==before);
        assert(g_calls== (trigger ? std::vector<unsigned>{0x5D,0x28,0x7D0,0} : std::vector<unsigned>{0x5D,0x28,0}));
        assert(g_queried==(trigger && (swap_flag || swap_effect) ? g_other.data() : g_actor.data()));
        std::uint32_t bits=0; std::memcpy(&bits,g_event.data()+0x190,4); assert(bits==0xA5);
        ++cases;
    }
    g_swap_flag=g_swap_effect=false; g_throw=true; g_calls.clear(); g_queried=nullptr;
    put(g_action.data(),0x28,g_actor.data()); put(g_event.data(),0x190,std::uint32_t{2});
    bool threw=false;
    try { update.update(g_action.data()); } catch(const std::runtime_error&) { threw=true; }
    std::uint32_t bits=1; std::memcpy(&bits,g_event.data()+0x190,4);
    assert(threw && bits==0 && !g_queried);
    assert(store.retire(token)); g_calls.clear();
    assert(update.update(g_action.data()).status==StormboltSpecialActionStatus::unavailable && g_calls.empty());
    assert(store.initialize(reinterpret_cast<std::uintptr_t>(g_weapon.data())));
    StormboltGatlingEntryApi entry_api{};
    entry_api.animation.motion=motion; entry_api.local_player=local_player;
    entry_api.player_manager_slot=&g_manager_slot; entry_api.ammo_items=g_ammo_items.data();
    StormboltGatlingStartEntry<1> entry(store,entry_api);
    unsigned entry_cases=0;
    for(std::uint32_t id : {238u,239u}) for(bool local : {false,true}) for(bool swap : {false,true}) {
        g_local=local; g_motion_swap=swap; g_motions=g_item_reads=0;
        put(g_action.data(),0x28,g_actor.data()); put(g_weapon.data(),0x9FC,id);
        put(g_weapon.data(),0x2350,std::uint8_t{1}); put(g_weapon.data(),0x27C8,std::uint32_t{40});
        put(g_weapon.data(),0x27DC,std::uint32_t{171});
        auto expected=g_weapon;
        put(expected.data(),0x2350,std::uint8_t{0}); put(expected.data(),0x27C8,std::uint32_t{local ? 33u : 0u});
        assert(entry.enter(g_action.data())==StormboltSpecialActionStatus::processed);
        assert(g_motions==3 && g_item_reads==1 && g_weapon==expected);
        ++entry_cases;
    }
    g_local=true; g_motion_swap=false;
    for (const auto item : {0u,137u,169u,171u,172u,0xFFFFFFFFu}) {
        g_motions=0; put(g_action.data(),0x28,g_actor.data()); put(g_weapon.data(),0x27DC,item);
        assert(entry.enter(g_action.data())==StormboltSpecialActionStatus::processed);
        std::uint32_t ammo=0; std::memcpy(&ammo,g_weapon.data()+0x27C8,4);
        assert(ammo==(item==0 ? 255u : item==169 ? 36u : item==171 ? 33u : 0u));
    }
    auto incomplete=entry_api; incomplete.ammo_items=nullptr;
    StormboltGatlingStartEntry<1> unavailable(store,incomplete);
    const auto before=g_weapon;
    assert(unavailable.enter(g_action.data())==StormboltSpecialActionStatus::unavailable && g_weapon==before);
    std::printf("PASS: Gatling start update / %u event, result and actor-reread cases; no game activation\n",cases);
    std::printf("PASS: Gatling start entry / %u local-owner and motion-reread cases; native table selection; game bootstrap pending\n",entry_cases);
}
