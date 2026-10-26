#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <array>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_focus_pendant.h"
int main() {
    std::array<unsigned char,19> bytes{};
    std::ifstream source("research/horizon-complete-c2-live-failures-20261021/focus-exclusion-original.bin",std::ios::binary);
    if(!source.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return 1;
    auto* memory=VirtualAlloc(nullptr,bytes.size(),MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if(!memory)return 2;
    std::memcpy(memory,bytes.data(),bytes.size());
    if(!FlushInstructionCache(GetCurrentProcess(),memory,bytes.size()))return 3;
    const auto original=reinterpret_cast<bool(*)(std::int32_t)>(memory);
    unsigned changed=0;
    for(std::int32_t row=-1;row<=65535;++row) {
        const bool before=original(row);
        const bool after=horizon_other::focus_pendant_exclusion_result(row,before);
        const bool focus=row==87 || row==98 || row==99 || row==100;
        if(before!=focus || after || (!focus && horizon_other::focus_pendant_exclusion_result(row,true)!=true))return 4;
        changed+=before!=after;
    }
    if(changed!=4 || !VirtualFree(memory,0,MEM_RELEASE))return 5;
    std::cout<<"PASS original native predicate executed for65537 rows; exactly four Focus exclusions removed, unrelated exclusion results preserved. No game installed.\n";
}
