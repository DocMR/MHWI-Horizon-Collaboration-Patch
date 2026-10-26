#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_charge_hud.h"
#include "stormbolt_charge_hud_hook.h"
#include "stormbolt_charge_hud_native_input.h"
#include "stormbolt_operation_hint_hook.h"
#include "stormbolt_itemwindow_hook.h"
#include "stormbolt_ordinary_reticle_hook.h"
#include "generated_charge_hud_animation_cases.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace stormslinger;
namespace {
struct Node { float frame=0; bool hidden=false; } ordinary,root,child,on_off;
std::array<Node,9> bars;
std::array<Node,2> selected;
std::array<Node,8> segments;
std::uint32_t active=0;
std::vector<std::pair<void*,std::uint32_t>> plays;
int context=0;
bool selected_target=false;
unsigned original_calls=0;
bool local_authority=true;
bool hint_source=false,hint_loaded=false;
bool window_source=false,window_drawn=false,window_selected=false;
bool ordinary_allowed=true,ordinary_target=true;
unsigned ordinary_admission_calls=0;
StormboltChargeHudInput ordinary_input{};
bool ordinary_admission(void*) {++ordinary_admission_calls;return ordinary_allowed;}
bool ordinary_mode(void*,StormboltChargeHudInput& out) {out=ordinary_input;return ordinary_target;}
Node window_change,window_bow,window_light,window_arrow,window_gauge_layout,window_ammo_layout;
unsigned window_original_calls=0;
void window_original(void*) {
    ++window_original_calls;
    window_bow.hidden=true;window_light.hidden=false;window_arrow.hidden=true;
    window_gauge_layout.frame=window_ammo_layout.frame=1.0f;
}
bool window_input(void*,StormboltChargeHudInput& output) {
    output.weapon_drawn=window_drawn;output.special_ammo_selected=window_selected;return window_source;
}
unsigned hint_original_calls=0;
bool hint_mode(bool& loaded) {loaded=hint_loaded;return hint_source;}
void hint_original(void*,void* parameters) {
    ++hint_original_calls;
    auto* values=static_cast<std::uint32_t*>(parameters);
    values[1]=0x12345678; // inherited native key glyph/style data
}
std::uint8_t authority(void*,std::uint32_t index) {assert(index==0);return local_authority?1:0;}
void original_refresh(void*) {
    ++original_calls;ordinary.hidden=false;root.hidden=true;
}
bool hook_input(void*,StormboltChargeHudInput& output) {
    output={true,50,100,true,true};return selected_target;
}
float fp(std::uint32_t value) {float result;std::memcpy(&result,&value,4);return result;}
void hidden(void* p,bool value) {static_cast<Node*>(p)->hidden=value;}
void frame(void* p,float value,std::uint8_t mode) {
    assert(mode==1);static_cast<Node*>(p)->frame=value;
}
float get_frame(void* p) {return static_cast<Node*>(p)->frame;}
std::uint32_t animation(void* p) {assert(p==&root);return active;}
bool play(void* p,std::uint32_t value,void* ctx) {
    assert(ctx==&context);plays.emplace_back(p,value);
    return true;
}
template<bool Training> struct HintRefreshFixture {
    inline static unsigned calls=0,rebuilds=0;
    inline static bool deferred=false;
    inline static std::uint32_t label=397;
    inline static std::array<void*,9> widget{};
    static constexpr std::size_t dirty=Training?0x38D4:0x6371;
    static void update(void* window) {
        ++calls;
        auto& flag=static_cast<unsigned char*>(window)[dirty];
        if(!flag || deferred) return;
        std::array<std::uint32_t,6> parameters{{397,0,2,3,4,5}};
        reinterpret_cast<void(*)(void*,void*)>(widget[8])(window,parameters.data());
        label=parameters[0];flag=0;++rebuilds;
    }
};
template<bool Training> void verify_hint_refresh() {
    using F=HintRefreshFixture<Training>;
    alignas(8) std::array<void*,77> table{};
    std::array<unsigned char,0x6372> window{},replacement{};
    F::widget[8]=reinterpret_cast<void*>(&hint_original);
    table[76]=reinterpret_cast<void*>(&F::update);
    StormboltOperationHintHook<Training> hook;
    assert(hook.install({reinterpret_cast<std::uintptr_t>(F::widget.data()),hint_original,hint_mode,
        reinterpret_cast<std::uintptr_t>(table.data()),&F::update}));
    auto tick=[&](auto& object){reinterpret_cast<void(*)(void*)>(table[76])(object.data());};
    hint_source=false;hint_loaded=false;tick(window);assert(F::rebuilds==0);
    hint_source=true;tick(window);assert(F::label==395 && F::rebuilds==1);
    tick(window);assert(F::rebuilds==1); // unchanged state must not restart animations
    hint_loaded=true;F::deferred=true;tick(window);
    assert(F::label==395 && window[F::dirty]==1 && F::rebuilds==1);
    F::deferred=false;tick(window);assert(F::label==406 && F::rebuilds==2);
    hint_source=false;tick(window);assert(F::label==397 && F::rebuilds==3);
    tick(replacement);assert(F::rebuilds==3); // unrelated/new native window unchanged
    hint_source=true;tick(replacement);assert(F::label==406 && F::rebuilds==4);
    assert(F::calls==8);
    assert(hook.restore());
    assert(table[76]==reinterpret_cast<void*>(&F::update) &&
        F::widget[8]==reinterpret_cast<void*>(&hint_original));
    // If the second slot cannot be installed, keep the already published
    // label owner until explicit rollback; never replace the foreign update.
    auto foreign=+[](void*){++F::calls;};
    table[76]=reinterpret_cast<void*>(foreign);
    StormboltOperationHintHook<Training> refused;
    assert(!refused.install({reinterpret_cast<std::uintptr_t>(F::widget.data()),hint_original,hint_mode,
        reinterpret_cast<std::uintptr_t>(table.data()),&F::update}));
    assert(F::widget[8]!=reinterpret_cast<void*>(&hint_original));
    assert(refused.restore() && table[76]==reinterpret_cast<void*>(foreign) &&
        F::widget[8]==reinterpret_cast<void*>(&hint_original));
    hint_source=false;hint_loaded=false;
}
}
int main() {
    StormboltChargeHudNodes nodes{};
    nodes.ordinary_root=&ordinary;nodes.charge_root=&root;
    nodes.charge_child=&child;nodes.on_off=&on_off;nodes.animation_context=&context;
    for(std::size_t i=0;i<bars.size();++i) nodes.bars[i]=&bars[i];
    for(std::size_t i=0;i<selected.size();++i) nodes.selected_ammo[i]=&selected[i];
    for(std::size_t i=0;i<segments.size();++i) nodes.segments[i]=&segments[i];
    const StormboltChargeHudApi api{hidden,frame,get_frame,animation,play};
    for(const auto& test:charge_hud_animation_cases) {
        bars[0].frame=fp(test.previous);plays.clear();
        active=test.storage==2?0xffffffffu:test.active;
        const StormboltChargeHudInput input{true,fp(test.current),100.0f,true,true};
        refresh_stormbolt_charge_hud(nodes,input,api);
        if(plays.size()!=test.count || (!plays.empty() &&
            (plays.front().second!=test.animation ||
             (plays.back().first==&child)!=test.child))) {
            std::printf("Mismatch prev=%08X current=%08X active=%08X storage=%u\n",
                test.previous,test.current,test.active,test.storage);return 1;
        }
        assert(ordinary.hidden && !root.hidden);
        for(const auto& node:segments) assert(node.frame==2.0f);
        for(const auto& node:selected) assert(node.frame==0.0f);
        for(const auto& node:bars) assert(node.frame==fp(test.current) ||
            (std::isnan(node.frame) && std::isnan(fp(test.current))));
    }
    // The owner's absent configuration must produce an empty bar, even with
    // nonzero stale charge. Ammo selection and draw state are independent.
    for(bool special:{false,true}) for(bool drawn:{false,true}) {
        bars[0].frame=50;plays.clear();
        refresh_stormbolt_charge_hud(nodes,{false,50,0,special,drawn},api);
        for(const auto& node:bars) assert(node.frame==0);
        for(const auto& node:selected) assert(node.frame==(special&&drawn?0.0f:1.0f));
    }
    // Exercise the real owned virtual slot: native refresh runs exactly once,
    // target rendering follows it, removing part/target inherits native reset.
    alignas(8) std::array<void*,13> table{};
    std::array<unsigned char,0x1B0> controller{};
    auto put=[&](std::size_t offset,void* value) {
        std::memcpy(controller.data()+offset,&value,sizeof(value));
    };
    table[12]=reinterpret_cast<void*>(&original_refresh);
    put(0,table.data());put(0x70,&context);put(0xC8,&ordinary);
    put(0xF0,&root);put(0xF8,&child);put(0x100,&on_off);
    for(std::size_t i=0;i<bars.size();++i) put(0x108+i*8,&bars[i]);
    for(std::size_t i=0;i<selected.size();++i) put(0x150+i*8,&selected[i]);
    for(std::size_t i=0;i<segments.size();++i) put(0x168+i*8,&segments[i]);
    StormboltChargeHudHook hook;
    const StormboltChargeHudBindings bindings{&table[12],original_refresh,
        reinterpret_cast<std::uintptr_t>(table.data()),hook_input,api};
    assert(hook.install(bindings));
    auto refresh=[&] {reinterpret_cast<void(*)(void*)>(table[12])(controller.data());};
    selected_target=true;refresh();
    assert(original_calls==1 && ordinary.hidden && !root.hidden);
    selected_target=false;refresh();
    assert(original_calls==2 && !ordinary.hidden && root.hidden);
    selected_target=true;put(0x108,nullptr);refresh();
    assert(original_calls==3 && !ordinary.hidden && root.hidden);
    assert(hook.restore());
    assert(table[12]==reinterpret_cast<void*>(&original_refresh));
    std::array<unsigned char,0x7820> actor{};
    std::array<unsigned char,0x2850> weapon{};
    std::array<unsigned char,0x9C4> parameter{};
    auto write=[](auto& object,std::size_t offset,auto value) {
        std::memcpy(object.data()+offset,&value,sizeof(value));
    };
    const auto controller_vt=reinterpret_cast<std::uintptr_t>(table.data());
    const auto actor_vt=controller_vt+0x100;
    const auto weapon_vt=controller_vt+0x200;
    const auto parameter_vt=controller_vt+0x300;
    put(0x58,actor.data());write(actor,0,actor_vt);write(weapon,0,weapon_vt);
    write(parameter,0,parameter_vt);
    write(actor,0xC,std::uint8_t{2});write(weapon,0xC,std::uint8_t{2});
    write(actor,0x76B0,weapon.data());write(weapon,0x9B0,actor.data());
    write(actor,0x7810,parameter.data());write(weapon,0x9F8,std::uint32_t{13});
    write(weapon,0x9FC,std::uint32_t{239});write(weapon,0x20E0,std::uint8_t{1});
    write(weapon,0x2388,std::uint32_t{1});write(weapon,0x27C8,std::uint32_t{40});
    write(actor,0x76A8,std::uint8_t{1});write(weapon,0x2358,75.0f);
    write(parameter,0x9C0,100.0f);
    void* authority_object=&context;
    const StormboltChargeHudIdentity identity{controller_vt,actor_vt,weapon_vt,parameter_vt,
        {&authority_object,authority}};
    StormboltChargeHudInput native_input{};
    assert(stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    assert(native_input.configuration_available && native_input.charge==75 &&
        native_input.maximum_charge==100 && native_input.special_ammo_selected && native_input.weapon_drawn);
    write(weapon,0x2388,std::uint32_t{0});
    assert(!stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    write(weapon,0x2388,std::uint32_t{1});write(weapon,0xC,std::uint8_t{0x10});
    assert(!stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    write(weapon,0xC,std::uint8_t{2});
    write(weapon,0x2388,std::uint32_t{1});write(weapon,0x9FC,std::uint32_t{237});
    assert(!stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    write(weapon,0x9FC,std::uint32_t{238});local_authority=false;
    assert(!stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    local_authority=true;write(actor,0x7810,static_cast<void*>(nullptr));
    assert(stormbolt_charge_hud_native_input(controller.data(),identity,native_input));
    assert(!native_input.configuration_available);
    alignas(8) std::array<void*,9> hint_table{};
    hint_table[8]=reinterpret_cast<void*>(&hint_original);
    StormboltOperationHintHook<> hint_hook;
    assert(hint_hook.install({reinterpret_cast<std::uintptr_t>(hint_table.data()),hint_original,hint_mode}));
    auto hint=[&](std::uint32_t label) {
        std::array<std::uint32_t,6> values{{label,0,2,3,4,5}};
        reinterpret_cast<void(*)(void*,void*)>(hint_table[8])(hint_table.data(),values.data());
        assert(values[1]==0x12345678 && values[2]==2 && values[3]==3 && values[4]==4 && values[5]==5);
        return values[0];
    };
    assert(hint(397)==397);
    hint_source=true;assert(hint(397)==395);
    hint_loaded=true;assert(hint(397)==406);
    for(std::uint32_t label=0;label<563;++label)
        if(label!=397) assert(hint(label)==label);
    hint_source=false;assert(hint(397)==397);
    assert(hint_original_calls==566);
    alignas(8) std::array<void*,9> training_table{};
    training_table[8]=reinterpret_cast<void*>(&hint_original);
    StormboltOperationHintHook<true> training_hook;
    assert(training_hook.install({reinterpret_cast<std::uintptr_t>(training_table.data()),hint_original,hint_mode}));
    hint_source=true;hint_loaded=false;
    std::array<std::uint32_t,6> training_values{{397,0,2,3,4,5}};
    reinterpret_cast<void(*)(void*,void*)>(training_table[8])(training_table.data(),training_values.data());
    assert(training_values[0]==395 && hint(397)==395);
    assert(training_hook.restore() && training_table[8]==reinterpret_cast<void*>(&hint_original));
    hint_loaded=true;assert(hint(397)==406);
    assert(hint_hook.restore() && hint_table[8]==reinterpret_cast<void*>(&hint_original));
    alignas(8) std::array<void*,77> window_table{};
    window_table[76]=reinterpret_cast<void*>(&window_original);
    std::array<unsigned char,0x3CD8> window{};
    auto window_put=[&](std::size_t offset,void* value) {
        std::memcpy(window.data()+offset,&value,sizeof(value));
    };
    window_put(0,window_table.data());window_put(0x3CB0,&window_change);
    window_put(0x3CA8,&window_gauge_layout);window_put(0x3CB8,&window_ammo_layout);
    window_put(0x3CC0,&window_bow);window_put(0x3CC8,&window_light);window_put(0x3CD0,&window_arrow);
    StormboltItemWindowHook window_hook;
    assert(window_hook.install({reinterpret_cast<std::uintptr_t>(window_table.data()),window_original,window_input,api}));
    auto window_refresh=[&]{reinterpret_cast<void(*)(void*)>(window_table[76])(window.data());};
    window_refresh();assert(window_bow.hidden && !window_light.hidden && window_arrow.hidden);
    window_source=true;window_drawn=false;window_refresh();
    assert(window_bow.hidden && window_light.hidden && !window_arrow.hidden && window_change.frame==0);
    window_drawn=true;window_refresh();assert(!window_bow.hidden && window_light.hidden && !window_arrow.hidden);
    for(bool drawn:{false,true}) for(bool special:{false,true}) {
        window_drawn=drawn;window_selected=special;window_refresh();
        assert(window_gauge_layout.frame==(special?0.0f:1.0f));
        assert(window_ammo_layout.frame==(special?0.0f:1.0f));
        assert(window_bow.hidden==!drawn && !window_arrow.hidden && window_light.hidden);
    }
    window_source=false;window_refresh();assert(window_bow.hidden && !window_light.hidden && window_arrow.hidden);
    assert(window_gauge_layout.frame==1.0f && window_ammo_layout.frame==1.0f);
    assert(window_original_calls==8);
    assert(window_hook.restore() && window_table[76]==reinterpret_cast<void*>(&window_original));
    verify_hint_refresh<false>();verify_hint_refresh<true>();
    std::array<void*,0x300/8> ordinary_table{};
    ordinary_table[0x2F8/8]=reinterpret_cast<void*>(&ordinary_admission);
    StormboltOrdinaryReticleHook ordinary_hook;
    assert(ordinary_hook.install({reinterpret_cast<std::uintptr_t>(ordinary_table.data()),
        ordinary_admission,ordinary_mode}));
    auto admission=reinterpret_cast<bool(*)(void*)>(ordinary_table[0x2F8/8]);
    ordinary_input.special_ammo_selected=true;ordinary_input.weapon_drawn=true;
    assert(!admission(nullptr));
    ordinary_input.special_ammo_selected=false;assert(admission(nullptr));
    ordinary_input.special_ammo_selected=true;ordinary_input.weapon_drawn=false;
    assert(admission(nullptr));
    ordinary_target=false;ordinary_input.weapon_drawn=true;assert(admission(nullptr));
    ordinary_allowed=false;assert(!admission(nullptr));
    assert(ordinary_admission_calls==5);
    assert(ordinary_hook.restore());
    assert(ordinary_table[0x2F8/8]==reinterpret_cast<void*>(&ordinary_admission));
    std::puts("PASS:ordinary reticle admission excludes drawn battery arrow mode; other states/native denial and exact restoration preserved");
    std::puts("PASS:1500 actual PS animation instruction cases; node frame writes, selection/draw and absent configuration");
    std::puts("PASS:real owned HUD slot, original once, target removal/native restoration, missing node and exact slot restoration");
    std::puts("PASS:native input target IDs, ownership, part8, authority and absent configuration");
    std::puts("PASS:Operations owned label hook, source load/unload, all other labels and native glyph/style preservation");
    std::puts("PASS:ItemWindow arrow-mode nodes, holstered/drawn, part removal and original-once restoration");
    std::puts("PASS:both hint windows mode-change invalidation, native deferred rebuild, unchanged state and part removal");
}
