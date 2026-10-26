#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace stormslinger {
using QuestLoaderOwnerRead=bool(*)(std::uintptr_t,void*,std::size_t);
// Only reconstruct a private owner-verification copy. Full original SHA remains
// mandatory in the caller. The caller authenticates the QuestLoader file first.
inline bool reconstruct_quest_loader_owner(std::uintptr_t image,std::uintptr_t loader,
    std::uint32_t owner_rva,std::uint8_t* bytes,std::size_t size,QuestLoaderOwnerRead read) {
    struct Site {std::uint32_t rva,hook;};
    constexpr Site sites[]={{0xF2C320,0x15FB0},{0x1976110,0x15E90},{0x19751B0,0x15F60},
        {0x478A20,0x16050},{0x1A10170,0x16000},{0x1355C40,0x15ED0},{0x1355C50,0x15F20}};
    if(!image || !loader || !bytes || !read)return false;
    bool changed=false;
    for(const auto& site:sites) {
        if(site.rva<owner_rva || site.rva-owner_rva>size || size-(site.rva-owner_rva)<5)continue;
        auto* prefix=bytes+(site.rva-owner_rva);if(prefix[0]!=0xE9)continue;
        std::uintptr_t hooks=0,trampoline=0,relay=0;unsigned count=0,found=0;
        if(!read(loader+0x776E0,&hooks,8) || !read(loader+0x776EC,&count,4) || !hooks || count>256)return false;
        for(unsigned i=0;i<count;++i) {
            std::uint8_t entry[0x38]{};if(!read(hooks+i*0x38,entry,sizeof(entry)))return false;
            std::uintptr_t target=0;std::memcpy(&target,entry,8);if(target!=image+site.rva)continue;
            if(!(entry[0x20]&2))return false;
            std::memcpy(&relay,entry+8,8);std::memcpy(&trampoline,entry+16,8);++found;
        }
        if(found!=1 || !relay || !trampoline)return false;
        std::int32_t displacement=0;std::memcpy(&displacement,prefix+1,4);
        if(static_cast<std::intptr_t>(image+site.rva+5)+displacement!=static_cast<std::intptr_t>(relay))return false;
        std::uint8_t branch[14]{},original[5]{};std::uintptr_t destination=0;
        constexpr std::uint8_t indirect[]={0xFF,0x25,0,0,0,0};
        if(!read(relay,branch,sizeof(branch)) || !read(trampoline,original,sizeof(original)))return false;
        std::memcpy(&destination,branch+6,8);
        if(std::memcmp(branch,indirect,6) || destination!=loader+site.hook)return false;
        std::memcpy(prefix,original,5);changed=true;
    }
    return changed;
}
}
