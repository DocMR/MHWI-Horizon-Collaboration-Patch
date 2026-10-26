#include "owned_code_word.h"
#include <cstdio>
#include <cstring>
struct FaultApi:stormslinger::CodeWordApi {
 static inline int protects=0,failProtectAt=0,failFlushes=0;
 static BOOL protect(void* p,SIZE_T n,DWORD f,DWORD* old) {
  if(++protects==failProtectAt)return FALSE;
  return CodeWordApi::protect(p,n,f,old);
 }
 static BOOL flush(void* p,SIZE_T n) {
  if(failFlushes>0){--failFlushes;return FALSE;}
  return CodeWordApi::flush(p,n);
 }
};
int main() {
 auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(!page)return 1;
 const unsigned char code[]={0xb8,11,0,0,0,0xc3};std::memcpy(page,code,sizeof(code));
 DWORD old=0;if(!VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old))return 2;
 auto call=reinterpret_cast<int(*)()>(page);if(call()!=11)return 3;
 stormslinger::OwnedCodeWord patch;
 if(patch.install(page+1,0,1) || patch.install(page,0x1234,0x5678))return 4;
 if(!patch.install(page,0x00000bb8,0x000016b8) || !patch.active() || call()!=22)return 5;
 MEMORY_BASIC_INFORMATION info{};VirtualQuery(page,&info,sizeof(info));if(info.Protect!=PAGE_EXECUTE_READ)return 6;
 if(!patch.restore() || patch.active() || call()!=11 || std::memcmp(page,code,sizeof(code)))return 7;
 if(!patch.restore())return 8;
 // Failed maintenance after a successful byte update must remain recoverable.
 stormslinger::OwnedCodeWordImpl<FaultApi> fault;
 FaultApi::protects=0;FaultApi::failProtectAt=2;
 if(fault.install(page,0x00000bb8,0x000016b8) || !fault.active())return 13;
 if(fault.install(page,0x000016b8,0x000021b8))return 14;
 FaultApi::failProtectAt=0;
 if(!fault.restore() || fault.active() || call()!=11)return 15;
 FaultApi::failFlushes=1;
 if(fault.install(page,0x00000bb8,0x000016b8) || !fault.active())return 16;
 if(!fault.restore() || fault.active() || call()!=11)return 17;
 // A restore that wrote original bytes but failed permission recovery remains
 // active until maintenance retry completes, even though no patched word remains.
 if(!fault.install(page,0x00000bb8,0x000016b8))return 18;
 FaultApi::failProtectAt=FaultApi::protects+2;
 if(fault.restore() || !fault.active())return 19;
 FaultApi::failProtectAt=0;
 if(!fault.restore() || fault.active() || call()!=11)return 20;
 if(!patch.install(page,0x00000bb8,0x000016b8))return 9;
 if(!VirtualProtect(page,4096,PAGE_EXECUTE_READWRITE,&old))return 10;
 InterlockedExchange(reinterpret_cast<volatile LONG*>(page),0x000021b8);
 if(!VirtualProtect(page,4096,old,&old))return 11;
 if(patch.restore() || !patch.active() || call()!=33)return 12;
 std::puts("PASS: alignment/mismatch rejection, executable behavior, exact restore, protection, foreign-write conflict retained");
 // A failed restore deliberately retains ownership; this synthetic allocation
 // remains until process exit, matching the no-unload-on-conflict requirement.
}
