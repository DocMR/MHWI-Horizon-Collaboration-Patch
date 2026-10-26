#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_swap_energy_hooks.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
std::array<unsigned char,0xEF00> fixture_actor{};
std::array<unsigned char,0x2860> fixture_weapon_a{},fixture_weapon_b{};
std::array<unsigned char,0x1770> fixture_parameter{};
std::array<unsigned char,0x40> snap{};
constexpr std::uintptr_t snapshot_vt=0x1234,weapon_vt=0x2345,param_vt=0x3456;
int saves=0,restores=0,ticks=0;std::uint32_t destroyed=0;
template<class T>void put(void* p,std::size_t off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
template<class T>T get(void* p,std::size_t off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
void* ctor(void* p){put(p,0,snapshot_vt);return p;}
void reset(void*){}
void* destroy(void* p,std::uint32_t flags){destroyed=flags;return p;}
void tick(void*,float){++ticks;}
void save(void* w){++saves;put(snap.data(),0x38,get<std::uint32_t>(w,0x2848));put(snap.data(),0x1C,get<float>(w,0x284C));}
void restore(void* w){++restores;put(w,0x2848,get<std::uint32_t>(snap.data(),0x38));put(w,0x284C,get<float>(snap.data(),0x1C));}
float focus(void*){return 1.2f;}
struct Transaction {
    StormboltSwapEnergyBindings callback{};unsigned count=0;
    MH_STATUS add(void* target,void* hook,void** original){
        *original=target;
        switch(count++){
        case 0:callback.construct=reinterpret_cast<decltype(callback.construct)>(hook);break;
        case 1:callback.reset=reinterpret_cast<decltype(callback.reset)>(hook);break;
        case 2:callback.destroy=reinterpret_cast<decltype(callback.destroy)>(hook);break;
        case 3:callback.tick=reinterpret_cast<decltype(callback.tick)>(hook);break;
        case 4:callback.save=reinterpret_cast<decltype(callback.save)>(hook);break;
        case 5:callback.restore=reinterpret_cast<decltype(callback.restore)>(hook);break;
        default:assert(false);
        }return MH_OK;
    }
};
}
int main(){
    StormboltSwapEnergyState zero{0.0f,0.0f};
    assert(restore_stormbolt_swap_energy(zero,100.0f,2.0f)==0.0f);
    advance_stormbolt_swap_energy(zero,20000.0f,1.2f);
    assert(zero.elapsed==10000.0f && restore_stormbolt_swap_energy(zero,100000.0f,2.0f)==20000.0f);

    put(fixture_actor.data(),0xC,std::uint8_t{0xE});put(fixture_actor.data(),0xEEB8,reinterpret_cast<std::uintptr_t>(snap.data()));
    put(fixture_actor.data(),0x7810,reinterpret_cast<std::uintptr_t>(fixture_parameter.data()));put(fixture_actor.data(),0x7D20,std::uintptr_t{1});
    put(fixture_parameter.data(),0,param_vt);put(fixture_parameter.data(),0x9C0,100.0f);put(fixture_parameter.data(),0x9CC,2.0f);
    for(auto* w:{fixture_weapon_a.data(),fixture_weapon_b.data()}){put(w,0,weapon_vt);put(w,0x9B0,reinterpret_cast<std::uintptr_t>(fixture_actor.data()));}
    StormboltSwapEnergyHooks<2> hooks;Transaction tx;
    assert(hooks.prepare(tx,{ctor,reset,destroy,tick,save,restore,focus,snapshot_vt,weapon_vt,param_vt})==MH_OK && tx.count==6);
    auto& c=tx.callback;assert(c.construct(snap.data())==snap.data());put(snap.data(),8,reinterpret_cast<std::uintptr_t>(fixture_actor.data()));
    c.restore(fixture_weapon_a.data());assert(get<float>(fixture_weapon_a.data(),0x2358)==100.0f);
    put(fixture_weapon_a.data(),0x2358,40.0f);put(fixture_weapon_a.data(),0x2848,std::uint32_t{1});put(fixture_weapon_a.data(),0x284C,7.0f);
    const auto before=snap;c.save(fixture_weapon_a.data());assert(get<std::uint32_t>(snap.data(),0x38)==1 && get<float>(snap.data(),0x1C)==7.0f);
    // Only the native stand-in writes original mine fields; no PS44/20 overwrite.
    auto expected=before;put(expected.data(),0x38,std::uint32_t{1});put(expected.data(),0x1C,7.0f);assert(snap==expected);
    c.tick(snap.data(),10.0f);c.restore(fixture_weapon_b.data());assert(get<float>(fixture_weapon_b.data(),0x2358)==64.0f);
    assert(get<std::uint32_t>(fixture_weapon_b.data(),0x2848)==1 && get<float>(fixture_weapon_b.data(),0x284C)==7.0f);
    // No battery condition: ordinary LBG swaps preserve the same independent energy.
    put(fixture_weapon_b.data(),0x2358,20.0f);c.save(fixture_weapon_b.data());c.tick(snap.data(),10000.0f);c.restore(fixture_weapon_a.data());assert(get<float>(fixture_weapon_a.data(),0x2358)==100.0f);
    c.reset(snap.data());c.restore(fixture_weapon_a.data());assert(get<float>(fixture_weapon_a.data(),0x2358)==100.0f);
    put(fixture_weapon_a.data(),0x2358,35.0f);c.save(fixture_weapon_a.data());put(fixture_weapon_a.data(),0x21D0,std::uint8_t{1});put(fixture_weapon_a.data(),0x2358,1.0f);c.save(fixture_weapon_a.data());
    c.restore(fixture_weapon_b.data());assert(get<float>(fixture_weapon_b.data(),0x2358)==35.0f);
    assert(c.destroy(snap.data(),5)==snap.data() && destroyed==5);put(fixture_weapon_b.data(),0x2358,17.0f);c.restore(fixture_weapon_b.data());assert(get<float>(fixture_weapon_b.data(),0x2358)==17.0f);
    assert(c.construct(snap.data())==snap.data());c.restore(fixture_weapon_b.data());assert(get<float>(fixture_weapon_b.data(),0x2358)==100.0f);
    hooks.withdraw_while_quiescent();put(fixture_weapon_b.data(),0x2358,9.0f);c.restore(fixture_weapon_b.data());assert(get<float>(fixture_weapon_b.data(),0x2358)==9.0f);
    hooks.release_after_stop();assert(saves==4 && ticks==2 && restores==8);
    std::puts("PASS swap energy: fresh/swap/Focus/cap/reset/skip/address-reuse/native-mine-preservation");
}
