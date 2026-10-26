#pragma once
#include <windows.h>
#include <cstdint>
namespace stormslinger {
struct CodeWordApi {
 static BOOL protect(void* p,SIZE_T n,DWORD flags,DWORD* old){return VirtualProtect(p,n,flags,old);}
 static BOOL flush(void* p,SIZE_T n){return FlushInstructionCache(GetCurrentProcess(),p,n);}
};
// Caller coordinates execution before changing executable instructions.
// This owns one aligned word and refuses to overwrite another writer's bytes.
template<class Api=CodeWordApi> class OwnedCodeWordImpl {
 volatile LONG* address=nullptr;
 LONG before=0,after=0;
 bool owned=false;
 bool protectionPending=false,flushPending=false;
 DWORD savedProtection=0;
 bool repairMaintenance() {
  bool complete=true;
  if(flushPending) {
   if(!Api::flush(const_cast<LONG*>(address),sizeof(LONG)))complete=false;
   else flushPending=false;
  }
  if(protectionPending) {
   DWORD unused=0;
   if(!Api::protect(const_cast<LONG*>(address),sizeof(LONG),savedProtection,&unused))return false;
   protectionPending=false;
  }
  return complete;
 }
 bool exchange(LONG expected,LONG desired) {
  if(!repairMaintenance())return false;
  DWORD protection=0;
  if(!Api::protect(const_cast<LONG*>(address),sizeof(LONG),PAGE_EXECUTE_READWRITE,&protection))return false;
  savedProtection=protection;protectionPending=true;
  bool matched=InterlockedCompareExchange(address,desired,expected)==expected;
  if(matched)owned=(desired==after);
  flushPending=matched;
  bool maintained=repairMaintenance();
  return matched && maintained;
 }
public:
 OwnedCodeWordImpl()=default;
 OwnedCodeWordImpl(const OwnedCodeWordImpl&)=delete;
 OwnedCodeWordImpl& operator=(const OwnedCodeWordImpl&)=delete;
 bool active() const noexcept{return owned || protectionPending || flushPending;}
 bool install(void* target,std::uint32_t expected,std::uint32_t replacement) {
  if(active() || !target || (reinterpret_cast<std::uintptr_t>(target)&3) || expected==replacement)return false;
  LONG live=0;SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),target,&live,sizeof(live),&read) || read!=sizeof(live) || static_cast<std::uint32_t>(live)!=expected)return false;
  address=static_cast<volatile LONG*>(target);before=static_cast<LONG>(expected);after=static_cast<LONG>(replacement);
  return exchange(before,after);
 }
 bool restore() {if(!repairMaintenance())return false;return !owned || exchange(after,before);}
};
using OwnedCodeWord=OwnedCodeWordImpl<>;
}
