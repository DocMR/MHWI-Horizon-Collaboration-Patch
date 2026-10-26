#pragma once
#include <windows.h>
#include <array>
#include <cstring>
#include <limits>
#include "weapon_tree_clone_spec.h"
namespace stormslinger {
// Process-lifetime storage: once published, never free while calls can be in flight.
class WeaponTreeClone {
 unsigned char* allocation=nullptr;
 const unsigned char* image=nullptr;
public:
 void* entry() const noexcept{return allocation;}
 bool prepare(const unsigned char* imageBase) {
  if(allocation)return imageBase==image;
  if(!imageBase)return false;
  // Resolve during component preparation, rather than extending the proxy's
  // static import table before the existing game loader finishes initialization.
  using AddFunctionTable=BOOLEAN (WINAPI*)(PRUNTIME_FUNCTION,DWORD,DWORD64);
  const auto ntdll=GetModuleHandleW(L"ntdll.dll");
  const auto addFunctionTable=ntdll?reinterpret_cast<AddFunctionTable>(
      GetProcAddress(ntdll,"RtlAddFunctionTable")):nullptr;
  if(!addFunctionTable)return false;
  std::array<unsigned char,sizeof(weaponTreeCloneBytes)> live{};
  SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),imageBase+0x13732f0,live.data(),live.size(),&read) || read!=live.size())return false;
  constexpr unsigned branchOffset=0x137348e-0x13732f0;
  const unsigned char excluded[]={0x0f,0x86,0x46,0x01,0x00,0x00};
  if(std::memcmp(live.data()+branchOffset,excluded,sizeof(excluded)))return false;
  std::memset(live.data()+branchOffset,0x90,sizeof(excluded));
  if(std::memcmp(live.data(),weaponTreeCloneBytes,live.size()))return false;
  auto source=reinterpret_cast<std::uintptr_t>(imageBase)+0x13732f0;
  unsigned char* memory=nullptr;
  // Search within 64 MiB; every displacement is checked rather than assumed.
  for(std::uintptr_t delta=0;delta<0x4000000 && !memory;delta+=0x10000) {
   for(int direction:{1,-1}) {
    std::uintptr_t candidate=(source&~std::uintptr_t(0xffff))+direction*delta;
    bool fits=true;
    for(const auto& r:weaponTreeCloneRelocations) {
     auto distance=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(imageBase)+r.targetRva)-static_cast<std::int64_t>(candidate+r.nextOffset);
     if(distance<(std::numeric_limits<std::int32_t>::min)() || distance>(std::numeric_limits<std::int32_t>::max)()){fits=false;break;}
    }
    if(fits)memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(candidate),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(memory)break;
   }
  }
  if(!memory)return false;
  std::memcpy(memory,weaponTreeCloneBytes,sizeof(weaponTreeCloneBytes));
  for(const auto& r:weaponTreeCloneRelocations) {
   auto distance=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(imageBase)+r.targetRva)-static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(memory)+r.nextOffset);
   auto value=static_cast<std::int32_t>(distance);std::memcpy(memory+r.offset,&value,sizeof(value));
  }
  constexpr DWORD unwindOffset=(sizeof(weaponTreeCloneBytes)+15)&~15u;
  std::memcpy(memory+unwindOffset,weaponTreeCloneUnwind,sizeof(weaponTreeCloneUnwind));
  constexpr DWORD tableOffset=(unwindOffset+sizeof(weaponTreeCloneUnwind)+15)&~15u;
  auto table=reinterpret_cast<RUNTIME_FUNCTION*>(memory+tableOffset);
  *table={0,static_cast<DWORD>(sizeof(weaponTreeCloneBytes)),unwindOffset};
  DWORD old=0;
  if(!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old) || !FlushInstructionCache(GetCurrentProcess(),memory,4096)){VirtualFree(memory,0,MEM_RELEASE);return false;}
  if(!addFunctionTable(table,1,reinterpret_cast<DWORD64>(memory))){VirtualFree(memory,0,MEM_RELEASE);return false;}
  allocation=memory;image=imageBase;return true;
 }
};
}
