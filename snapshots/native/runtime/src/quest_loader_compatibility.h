#pragma once
#include "quest_signatures.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stormslinger {
// Recognizes only the five observed QuestLoader detours. This edits a private
// validation copy, never the live game or the other plugin's trampoline.
using QuestCompatRead=bool(*)(std::uintptr_t,void*,std::size_t,void*);
inline bool normalize_known_quest_loader(std::uint8_t* text,std::size_t size,
    std::uintptr_t text_rva,std::uintptr_t image,std::uintptr_t loader,
    QuestCompatRead read,void* context,unsigned& normalized) {
    normalized=0;
    struct Site {unsigned signature,hook;};
    constexpr Site sites[]={{0,0x15fb0},{1,0x15e90},{2,0x15f60},{6,0x16050},{7,0x16000}};
    if(!text || !read)return false;
    for(const auto& site:sites) {
        const auto& signature=questSignatures[site.signature];
        if(signature.rva<text_rva)return false;
        const auto at=signature.rva-text_rva;
        if(at>size || size-at<5)return false;
        std::uint8_t prefix[5]{};std::size_t byte=0;
        const char* bit=signature.bits;bool matches=true;
        while(*bit) {
            while(*bit==' ')++bit;
            if(!*bit)break;
            unsigned value=0,mask=0;
            for(unsigned b=0;b<8;++b,++bit) {value=(value<<1)|(*bit=='1');mask=(mask<<1)|(*bit!='.');}
            if(byte>=size-at)return false;
            if(byte<5) {if(mask!=255)return false;prefix[byte]=static_cast<std::uint8_t>(value);}
            if((text[at+byte]&mask)!=value) {
                if(byte>=5 || text[at]!=0xe9)return false;
                matches=false;
            }
            ++byte;
        }
        if(matches)continue;
        if(!loader)return false;
        std::int32_t displacement=0;std::memcpy(&displacement,text+at+1,4);
        const auto relay=static_cast<std::uintptr_t>(static_cast<std::intptr_t>(image+signature.rva+5)+displacement);
        std::uint8_t branch[14]{};
        if(!read(relay,branch,sizeof(branch),context))return false;
        const std::uint8_t indirect[]={0xff,0x25,0,0,0,0};
        std::uintptr_t destination=0;std::memcpy(&destination,branch+6,8);
        if(std::memcmp(branch,indirect,6) || destination!=loader+site.hook)return false;
        std::memcpy(text+at,prefix,5);++normalized;
    }
    return true;
}
}
