#include "hooks.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace {
int replacement(){return 42;}
}
int main() {
 using namespace stormslinger;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,
     MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
 if(!code)return 1;
 const unsigned char mov[]={0x8B,0x05,0xFA,0,0,0,0xC3};
 const unsigned char lea[]={0x48,0x8D,0x05,0xD9,0,0,0,0xC3};
 const unsigned char ordinary[]={0xB8,0x0B,0,0,0,0xC3};
 std::memcpy(code,mov,sizeof(mov));
 std::memcpy(code+32,lea,sizeof(lea));
 std::memcpy(code+64,ordinary,sizeof(ordinary));
 const std::uint32_t values[]={73,91};
 std::memcpy(code+256,values,sizeof(values));
 auto read=reinterpret_cast<int(*)()>(code);
 auto address=reinterpret_cast<void*(*)()>(code+32);
 auto normal=reinterpret_cast<int(*)()>(code+64);
 if(read()!=73 || address()!=code+256 || normal()!=11)return 2;
 HookTransaction transaction;
 if(transaction.begin()!=MH_OK)return 3;
 void* original=nullptr;
 if(transaction.add(code+64,reinterpret_cast<void*>(&replacement),&original)!=MH_OK ||
    transaction.add_rip_patch(code,mov,6,2,254)!=MH_OK ||
    transaction.add_rip_patch(code+32,lea,7,3,221)!=MH_OK)return 4;
 if(read()!=73 || address()!=code+256 || normal()!=11)return 5;
 if(transaction.commit()!=MH_OK || read()!=91 || address()!=code+260 || normal()!=42)return 6;
 if(std::memcmp(code,mov,2) || std::memcmp(code+32,lea,3))return 7;
 // Foreign modification must retain transaction ownership for a retry.
 const auto owned=code[2];code[2]^=1;
 if(transaction.stop()==MH_OK || !transaction.has_library_state())return 8;
 code[2]=owned;
 if(transaction.stop()!=MH_OK || read()!=73 || address()!=code+256 || normal()!=11)return 9;
 if(std::memcmp(code,mov,sizeof(mov)) || std::memcmp(code+32,lea,sizeof(lea)))return 10;
 if(MH_Initialize()!=MH_OK)return 11;
 const unsigned char unsupported[]={0xE8,0,0,0,0,0};
 if(MH_CreateRipRelativePatch(code,unsupported,6,2,0)!=MH_ERROR_UNSUPPORTED_FUNCTION)return 12;
 unsigned char stale[6];std::memcpy(stale,mov,6);stale[2]^=1;
 if(MH_CreateRipRelativePatch(code,stale,6,2,0)!=MH_ERROR_UNSUPPORTED_FUNCTION)return 13;
 if(MH_Uninitialize()!=MH_OK || !VirtualFree(code,0,MEM_RELEASE))return 14;
 std::puts("PASS: mixed detour/RIP queue; native MOV/LEA results; exact restore; foreign-write retry; stale/opcode rejection");
 return 0;
}
