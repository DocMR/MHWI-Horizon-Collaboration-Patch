#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_shot_effects.h"
#include <cassert>
#include <cstdio>
#include <tuple>
#include <vector>
using namespace stormslinger;
namespace {
std::array<std::byte,0x2800> fixture_weapon{};
std::array<std::byte,0x8810> fixture_actor{};
std::array<std::byte,0x40> fixture_component{};
std::array<std::byte,0x1B60> fixture_buffer{};
std::array<std::byte,0x430> fixture_result{};
std::vector<std::tuple<unsigned,unsigned,unsigned>> fixture_requests;
unsigned fixture_stops=0,fixture_colors=0;
void* fixture_local{};void* fixture_manager=&fixture_actor;
template<class T>void put(void* p,std::size_t off,T value){std::memcpy(static_cast<std::byte*>(p)+off,&value,sizeof(value));}
template<class T>T get(const void* p,std::size_t off){T value{};std::memcpy(&value,static_cast<const std::byte*>(p)+off,sizeof(value));return value;}
bool active(const void* bank){return get<void*>(bank,0)!=nullptr;}
void stop(void*){++fixture_stops;}
void* assign(void* bank,void* effect){put(bank,0,effect);return effect;}
void* request(void*,unsigned group,unsigned record,unsigned attach){fixture_requests.emplace_back(group,record,attach);return fixture_result.data();}
unsigned attachment(void*){return 0xC9;}
void color(void*,const unsigned* value){assert(*value==ps_stormbolt_color_default);++fixture_colors;}
void* player(void*){return fixture_local;}
}
int main(){
 StormboltInstanceStore<2> store;auto token=store.initialize(reinterpret_cast<std::uintptr_t>(fixture_weapon.data()));
 put(fixture_weapon.data(),0x9B0,fixture_actor.data());put(fixture_weapon.data(),0x998,fixture_actor.data());
 put(fixture_actor.data(),0xC,std::uint8_t{2});put(fixture_actor.data(),0x8808,fixture_component.data());
 put(fixture_component.data(),8,fixture_buffer.data());put(fixture_result.data(),0xC,std::uint8_t{2});fixture_local=fixture_actor.data();
 StormboltShotEffectApi api{{active,request,assign},{active,stop,assign},{&fixture_manager,player},{active,color},attachment};
 constexpr unsigned fire[]={0,0,0,1,1,2,2,2,3};
 for(unsigned shot=0;shot<9;++shot){
  fixture_buffer.fill(std::byte{});fixture_requests.clear();fixture_colors=fixture_stops=0;
  store.transact(token,[&](auto& s) noexcept {s.counter=shot+1;s.effect_latch=0;return true;});
  assert(stormbolt_shot_effects(store,fixture_weapon.data(),shot,api));
  assert(fixture_requests.front()==std::make_tuple(0x7DAu,fire[shot],0xC9u));
  StormboltStageState state{};assert(store.snapshot(token,state));
  if(shot==8){assert(fixture_requests.size()==2 && fixture_requests[1]==std::make_tuple(0x7DBu,3u,0xFFFFFFFFu));assert(!state.effect_latch && !fixture_colors);}
  else if(shot>=5){assert(std::get<0>(fixture_requests.back())==0x834 && fixture_colors==1 && state.effect_latch && state.effect_alpha==255);}
  else {assert(!fixture_colors && !state.effect_latch);}
 }
 // Existing screen bank must not restart or reset its partial fade.
 fixture_buffer.fill(std::byte{});put(fixture_buffer.data(),0x1AA0,fixture_result.data());fixture_requests.clear();fixture_colors=0;
 store.transact(token,[](auto& s) noexcept {s.counter=7;s.effect_latch=1;s.effect_alpha=100;return true;});
 assert(stormbolt_shot_effects(store,fixture_weapon.data(),6,api));
 StormboltStageState state{};assert(store.snapshot(token,state) && state.effect_alpha==100 && !fixture_colors);
 for(auto [group,record,attach]:fixture_requests){(void)record;(void)attach;assert(group!=0x834);}
 // Another fixture_actor retains world shot effects but cannot create fixture_local screen tint.
 fixture_local=nullptr;fixture_buffer.fill(std::byte{});fixture_requests.clear();
 store.transact(token,[](auto& s) noexcept {s.counter=7;s.effect_latch=0;return true;});
 assert(stormbolt_shot_effects(store,fixture_weapon.data(),6,api));
 for(auto [group,record,attach]:fixture_requests){(void)record;(void)attach;assert(group!=0x834);}
 // Final shot clears a live tint and resets only its latch.
 fixture_local=fixture_actor.data();fixture_buffer.fill(std::byte{});put(fixture_buffer.data(),0x1AA0,fixture_result.data());fixture_stops=0;
 assert(stormbolt_shot_effects(store,fixture_weapon.data(),8,api));
 assert(fixture_stops==1 && !get<void*>(fixture_buffer.data(),0x1AA0));
 assert(store.retire(token));fixture_requests.clear();assert(!stormbolt_shot_effects(store,fixture_weapon.data(),1,api) && fixture_requests.empty());
 std::puts("PASS shot effects: nine stages, final discharge, fixture_local-only screen tint, retained bank/fade, final cleanup and retired instance");
}
