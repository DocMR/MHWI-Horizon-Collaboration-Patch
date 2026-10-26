#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_equipment_startup_gate.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace stormslinger;
namespace {
StormboltEquipmentStartupGate gate;
HookTransaction quest,shooting;
struct Manager {std::uintptr_t table;std::array<unsigned char,0x30> pad{};int phase=0;} manager,foreign;
static_assert(offsetof(Manager,phase)==0x38);
void* singleton=&manager;
unsigned calls=0,attempts=0,mode_seen=0;
void* manager_seen=nullptr;
bool retry=false;
__declspec(noinline) bool native(void* value,unsigned mode) {
 ++calls;manager_seen=value;mode_seen=mode;return mode==7;
}
using Fn=int(*)();
Fn quest_original=nullptr,shooting_original=nullptr;
unsigned char* page=nullptr;
int quest_hook() {return quest_original()+3;}
int shooting_hook() {return shooting_original()+5;}
int start(void*) {
 ++attempts;
 assert(calls==0 || retry);
 assert(manager.phase<4);
 if(retry && attempts==1) return -13;
 assert(quest.begin_shared_with(gate.transaction())==MH_OK);
 assert(quest.add(page,reinterpret_cast<void*>(&quest_hook),reinterpret_cast<void**>(&quest_original))==MH_OK);
 assert(quest.commit()==MH_OK);
 assert(shooting.begin_shared_with(quest)==MH_OK);
 assert(shooting.add(page+64,reinterpret_cast<void*>(&shooting_hook),reinterpret_cast<void**>(&shooting_original))==MH_OK);
 assert(shooting.commit()==MH_OK);
 return 0;
}
}
int main(int argc,char** argv) {
 assert(argc==2);std::string mode=argv[1];
 assert(mode=="success" || mode=="retry" || mode=="late" || mode=="foreign" || mode=="cancel" || mode=="already-late" || mode=="not-born" || mode=="create-failure" || mode=="begin-failure");
 manager.table=0x12345678;foreign.table=manager.table;retry=mode=="retry";
 page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(page);
 std::memset(page,0x90,4096);
 for(unsigned off:{0u,64u}) {page[off]=0xB8;page[off+1]=11;std::memset(page+off+2,0,3);page[off+5]=0xC3;}
 std::array<unsigned char,128> before{};std::memcpy(before.data(),page,before.size());
 if(mode=="already-late") manager.phase=4;
 if(mode=="not-born") singleton=nullptr;
 if(mode=="begin-failure") assert(MH_Initialize()==MH_OK);
 const auto target=mode=="create-failure"?reinterpret_cast<StormboltEquipmentStartupGate::Native>(&manager):&native;
 const auto armed=gate.arm(target,&singleton,manager.table,&start,nullptr);
 if(mode=="begin-failure" || mode=="create-failure") {
  assert(armed==(mode=="begin-failure"?MH_ERROR_ALREADY_INITIALIZED:MH_ERROR_NOT_EXECUTABLE));
  assert(gate.install_stage()==(mode=="begin-failure"?2:3) && gate.install_status()==armed);
  assert(!gate.armed() && !gate.transaction().has_library_state());
  if(mode=="begin-failure") assert(MH_Uninitialize()==MH_OK);
  assert(VirtualFree(page,0,MEM_RELEASE));
  std::puts("PASS native bootstrap failed stage preserved after cleanup");return 0;
 }
 if(mode=="already-late") {
  assert(armed==MH_ERROR_UNSUPPORTED_FUNCTION && !gate.armed() && !gate.transaction().has_library_state());
  assert(gate.install_stage()==1);
 } else {assert(armed==MH_OK);assert(gate.install_stage()==6 && gate.install_status()==MH_OK);}
 if(mode=="not-born") singleton=&manager;
 StormboltEquipmentStartupGate::Native volatile entry=&native;
 if(mode=="late") manager.phase=4;
 if(mode=="cancel") assert(gate.cancel_pending(-30)==-30);
 auto* object=mode=="foreign"?&foreign:&manager;
 assert(entry(object,7));assert(calls==1 && manager_seen==object && mode_seen==7);
 if(mode=="cancel") assert(gate.result()==-30 && attempts==0);
 else if(mode=="late" || mode=="already-late") assert(gate.result()==-19 && attempts==0);
 else if(mode=="foreign") assert(gate.result()==-13 && attempts==0);
 else {
  if(retry) {
   assert(gate.result()==-13 && attempts==1);
   manager.phase=3;assert(!entry(&manager,9));assert(mode_seen==9 && calls==2);
  }
  assert(gate.result()==0 && attempts==(retry?2u:1u));
  assert(gate.cancel_pending(-30)==0);
  assert(reinterpret_cast<Fn>(page)()==14 && reinterpret_cast<Fn>(page+64)()==16);
  assert(gate.transaction().stop()==MH_ERROR_ENABLED && quest.stop()==MH_ERROR_ENABLED);
  assert(shooting.stop()==MH_OK && quest.stop()==MH_OK);
 }
 assert(gate.transaction().stop()==MH_OK);
 assert(!std::memcmp(before.data(),page,before.size()));
 assert(entry(&manager,7));assert(VirtualFree(page,0,MEM_RELEASE));
 std::printf("PASS native equipment entry, original forwarding and nested lifetime / %s\n",mode.c_str());
}
