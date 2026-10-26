#ifdef NDEBUG
#undef NDEBUG
#endif
#include "quest_loader_owner_copy.h"
#include <map>
#include <vector>
#include <cassert>
#include <cstdio>
namespace {
std::map<std::uintptr_t,std::vector<std::uint8_t>> memory;
bool read(std::uintptr_t address,void* output,std::size_t bytes) {
    for(const auto& region:memory)if(address>=region.first && address-region.first<=region.second.size() &&
        bytes<=region.second.size()-(address-region.first)) {
        std::memcpy(output,region.second.data()+address-region.first,bytes);return true;
    }
    return false;
}
template<class T> void put(std::uintptr_t address,T value) {
    auto& b=memory[address];b.resize(sizeof(T));std::memcpy(b.data(),&value,sizeof(T));
}
}
int main() {
    constexpr std::uintptr_t image=0x10000000,loader=0x20000000,records=0x30000000,
        relay=0x10010000,trampoline=0x10020000;
    constexpr std::uint32_t rva=0x478A20;
    const std::uint8_t original[]={0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20};
    std::uint8_t copy[sizeof(original)]{};std::memcpy(copy,original,sizeof(copy));
    copy[0]=0xE9;auto delta=static_cast<std::int32_t>(relay-(image+rva+5));std::memcpy(copy+1,&delta,4);
    put(loader+0x776E0,records);put(loader+0x776EC,1u);
    auto& entry=memory[records];entry.resize(0x38);auto target=image+rva;
    std::memcpy(entry.data(),&target,8);std::memcpy(entry.data()+8,&relay,8);
    std::memcpy(entry.data()+16,&trampoline,8);entry[0x20]=2;
    auto& branch=memory[relay];branch={0xFF,0x25,0,0,0,0};branch.resize(14);
    auto destination=loader+0x16050;std::memcpy(branch.data()+6,&destination,8);
    memory[trampoline]=std::vector<std::uint8_t>(original,original+5);
    auto reset=[&]{std::memcpy(copy,original,sizeof(copy));copy[0]=0xE9;std::memcpy(copy+1,&delta,4);};
    assert(stormslinger::reconstruct_quest_loader_owner(image,loader,rva,copy,sizeof(copy),&read));
    assert(!std::memcmp(copy,original,sizeof(copy)));
    reset();branch[6]^=1;
    assert(!stormslinger::reconstruct_quest_loader_owner(image,loader,rva,copy,sizeof(copy),&read));branch[6]^=1;
    reset();entry[0x20]=0;
    assert(!stormslinger::reconstruct_quest_loader_owner(image,loader,rva,copy,sizeof(copy),&read));entry[0x20]=2;
    reset();copy[1]^=1;
    assert(!stormslinger::reconstruct_quest_loader_owner(image,loader,rva,copy,sizeof(copy),&read));
    reset();copy[8]^=1;
    assert(stormslinger::reconstruct_quest_loader_owner(image,loader,rva,copy,sizeof(copy),&read));
    assert(std::memcmp(copy,original,sizeof(copy))); // Full owner SHA must reject the unrelated tail change.
    reset();assert(!stormslinger::reconstruct_quest_loader_owner(image,loader,0x12345,copy,sizeof(copy),&read));
    std::puts("PASS QuestLoader owner private-copy reconstruction; foreign relay, disabled entry, wrong jump rejected; unrelated owner bytes remain changed for full SHA rejection");
}
