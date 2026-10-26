#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_shield_multiplier_call.h"
#include "../staging/Horizon-complete-integration-20261021/src/horizon_shield_reaction_call.h"
#include "../runtime/src/hooks.h"
#include "../runtime/src/stormbolt_power_cell_growth_commit.h"
#include "../runtime/src/stormbolt_power_cell_timer_adapter.h"
#include "../runtime/src/stormbolt_power_cell_lifecycle.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace {
int level=1;
unsigned removal_requests=0;
void prompt(void*,std::uint32_t skill,bool removal,std::int32_t value) {
 if(skill==0x1d3 && removal && value==-1) ++removal_requests;
}
std::int32_t query(void*,std::uint32_t id,std::uint32_t zero,std::int32_t fallback) {
 if(id!=0xd1 || zero || fallback) return 0;
 return level;
}
template<class T>void put(std::vector<std::uint8_t>& v,std::size_t off,T value) {std::memcpy(v.data()+off,&value,sizeof(value));}
}
int main() {
 using namespace horizon_other;using namespace stormslinger;
 auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,0x55d3000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
 std::ifstream m("research/horizon-complete-shield-multiplier-owner.bin",std::ios::binary),r("research/horizon-complete-shield-reaction-owner.bin",std::ios::binary);
 CHECK(m.read(reinterpret_cast<char*>(image+0x11b9c00),1198));CHECK(r.read(reinterpret_cast<char*>(image+0x1217ac0),903));
 const auto base=reinterpret_cast<std::uintptr_t>(image);
 std::array<unsigned char,7> original_m{};std::array<unsigned char,6> original_r{};
 std::memcpy(original_m.data(),image+0x11b9d44,7);std::memcpy(original_r.data(),image+0x1217b3e,6);
 std::vector<std::uint8_t> actor(0x12700),skill(0x1420),status(0x20),parameters(0xa98);
 void* resource=parameters.data();
 put(actor,0x60,std::uint8_t(1));put(actor,0x7d20,reinterpret_cast<std::uintptr_t>(status.data()));
 put(actor,0x126d8,reinterpret_cast<std::uintptr_t>(skill.data()));put(skill,0,base+0x324b1f0);put(skill,0x141c,std::int32_t(1));
 put(status,0,base+0x322c268);put(status,8,reinterpret_cast<std::uintptr_t>(actor.data()));
 put(actor,0x8950,std::uint32_t(0x80000000));put(actor,0x8948,std::uint32_t(0));
 put(parameters,0,base+0x31e0078);put(parameters,0xa80,0.5f);put(parameters,0xa84,0.75f);put(parameters,0xa88,1.0f);
 put(parameters,0xa8c,std::uint32_t(6));put(parameters,0xa90,std::uint32_t(2));put(parameters,0xa94,10.0f);
 StormboltLoopInputBindings inputs{base+0x324b1f0,base+0x322c268,reinterpret_cast<std::uintptr_t>(&resource),base+0x31e0078,0};
 StormboltPowerCellStore<64> shared;auto token=shared.initialize(reinterpret_cast<std::uintptr_t>(status.data()));CHECK(token);
 HookTransaction tx;CHECK(tx.begin()==MH_OK);
 ShieldMultiplierCall<64> multiplier(image,shared,inputs,&query);ShieldReactionCall<64> reaction(image,shared,inputs,&query);
 CHECK(multiplier.prepare_while_quiescent(tx));
 CHECK(reaction.prepare_while_quiescent(tx));
 CHECK(tx.commit_pending_without_rollback()==MH_OK);
 CHECK(multiplier.activate_after_shared_commit_while_quiescent());CHECK(reaction.activate_after_shared_commit_while_quiescent());
 CHECK(horizon_shield_multiplier_dispatch && horizon_shield_reaction_dispatch);
 auto count=[&](std::uint32_t value){return shared.transact(token,[&](auto& state)noexcept{state.count=value;return true;});};
 CHECK(count(0));CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==50);CHECK(horizon_shield_reaction_dispatch(status.data()));
 CHECK(count(4));CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==75);CHECK(horizon_shield_reaction_dispatch(status.data()));
 CHECK(count(6));CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);CHECK(!horizon_shield_reaction_dispatch(status.data()));
 CHECK(count(8));CHECK(horizon_shield_reaction_dispatch(status.data())); // Native != threshold, not < threshold.
 level=0;CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);CHECK(!horizon_shield_reaction_dispatch(status.data()));
 level=1;put(skill,0x141c,std::int32_t(0));CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);CHECK(!horizon_shield_reaction_dispatch(status.data()));
 put(skill,0x141c,std::int32_t(1));put(actor,0x8948,std::uint32_t(0x00800000));CHECK(!horizon_shield_reaction_dispatch(status.data()));
 put(actor,0x8948,std::uint32_t(0));put(actor,0x8950,std::uint32_t(0));CHECK(!horizon_shield_reaction_dispatch(status.data()));
 put(actor,0x8950,std::uint32_t(0x80000000));CHECK(shared.retire(token));
 CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);CHECK(!horizon_shield_reaction_dispatch(status.data()));
 auto fresh=shared.initialize(reinterpret_cast<std::uintptr_t>(status.data()));CHECK(fresh.generation!=token.generation);token=fresh;
 CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==50);
 // Exercise the shared native-input adapters with retained PC/PS file values,
 // rather than treating the arbitrary branch-test parameters above as retail.
 std::ifstream plain("research/pl_skill_param.decrypted.bin",std::ios::binary);
 plain.seekg(1891);CHECK(plain.read(reinterpret_cast<char*>(parameters.data()+0xa80),24));
 StormboltPowerCellParameters loaded{};CHECK(read_stormbolt_power_cell_parameters(parameters.data(),loaded));
 CHECK(loaded.threshold==4 && loaded.related_count==2 && loaded.duration==150.0f);
 CHECK(loaded.multipliers[0]==2 && loaded.multipliers[1]==2 && loaded.multipliers[2]==1);
 CHECK(shared.transact(token,[](auto& state)noexcept {state={};return true;}));
 put(actor,0x8938,std::uint8_t(1));
 std::vector<std::uint8_t> damage(0x68),event(0xc0),metadata(0xa4);
 constexpr std::uintptr_t damage_vtable=0x12345678;
 put(damage,0,damage_vtable);put(damage,0x60,reinterpret_cast<std::uintptr_t>(actor.data()));
 put(event,0xb8,reinterpret_cast<std::uintptr_t>(metadata.data()));
 put(metadata,0x20,1.0f);put(metadata,0xa0,std::uint32_t(0));
 StormboltDamageExceptionStore<64> damages;
 CHECK(damages.initialize(reinterpret_cast<std::uintptr_t>(damage.data())));
 float amount=1;
 auto increment=[&](StormboltPowerCellGrowthResult& result) {
  StormboltPowerCellGrowthInputs gathered{};
  if(!read_stormbolt_power_cell_growth_inputs(damage.data(),event.data(),&amount,
      damage_vtable,damages,shared,inputs,&query,gathered)) return false;
  // Local-player resolution is a native callback in production; this fixture
  // deliberately supplies only that identity, not an actual game manager.
  gathered.gate.associated_actor_is_local=true;
  return commit_stormbolt_power_cell_growth(gathered,shared,result);
 };
 for(unsigned n=1;n<=4;++n) {
  StormboltPowerCellGrowthResult result{};CHECK(increment(result));
  CHECK(result==(n<4 ? StormboltPowerCellGrowthResult::below_threshold : StormboltPowerCellGrowthResult::reached_threshold));
  StormboltPowerCellState state{};CHECK(shared.snapshot(token,state));
  CHECK(state.count==n && state.timer==(n<4 ? -1.0f : 150.0f));
 }
 CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);
 StormboltPowerCellGrowthResult saturated{};CHECK(increment(saturated));
 CHECK(saturated==StormboltPowerCellGrowthResult::unchanged);
 put(actor,0x8938,std::uint8_t(0));
 CHECK(advance_stormbolt_power_cell_timer_from_status(status.data(),149,shared,inputs,&query,&prompt));
 StormboltPowerCellState state{};CHECK(shared.snapshot(token,state));CHECK(state.timer==150);
 put(actor,0x8938,std::uint8_t(1));
 CHECK(advance_stormbolt_power_cell_timer_from_status(status.data(),149,shared,inputs,&query,&prompt));
 CHECK(shared.snapshot(token,state));CHECK(state.timer==1 && state.count==4 && removal_requests==0);
 CHECK(advance_stormbolt_power_cell_timer_from_status(status.data(),1,shared,inputs,&query,&prompt));
 CHECK(shared.snapshot(token,state));CHECK(state.timer==-1 && state.count==0 && removal_requests==1);
 CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==200);
 level=0;StormboltPowerCellGrowthResult no_skill{};CHECK(increment(no_skill));
 CHECK(no_skill==StormboltPowerCellGrowthResult::unchanged);
 CHECK(horizon_shield_multiplier_dispatch(status.data(),100)==100);
 level=1;
 StormboltPowerCellLifecycle<64> lifecycle(shared);
 CHECK(shared.transact(token,[](auto& value)noexcept {value.count=4;value.timer=150;return true;}));
 const auto old=token;
 token=lifecycle.reset(status.data(),[] {},[](void*) {return true;});CHECK(token);
 CHECK(token.generation!=old.generation && !shared.snapshot(old,state));
 CHECK(shared.snapshot(token,state) && state.count==0 && state.timer==-1 && !state.event_eligible);
 lifecycle.destroy(status.data(),[] {});CHECK(!shared.snapshot(token,state));
 CHECK(reaction.restore_while_quiescent());CHECK(multiplier.restore_while_quiescent());
 CHECK(!horizon_shield_multiplier_dispatch && !horizon_shield_reaction_dispatch);
 CHECK(tx.stop()==MH_OK);CHECK(reaction.release_after_shared_stop());CHECK(multiplier.release_after_shared_stop());
 CHECK(!std::memcmp(image+0x11b9d44,original_m.data(),7));CHECK(!std::memcmp(image+0x1217b3e,original_r.data(),6));
 CHECK(VirtualFree(image,0,MEM_RELEASE));
 std::cout<<"PASS shared shield callbacks: 3 parameter bands, any-weapon eligibility, absent/removed skill, reaction flags, stale generation, publication and exact CALL restoration. Retained retail parameters drive 4-event growth, saturation, enabled timer expiry/prompt, skill removal and reset/destruction. Actor/query/local identity/native step are fixtures; live armor effects and actual span register execution not tested.\n";
}
