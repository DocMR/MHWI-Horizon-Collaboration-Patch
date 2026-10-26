#pragma once
#include <cstring>
#include "weapon_tree_filter.h"
namespace stormslinger {
using TreeExcluded = bool(*)(void*, const void*);
using NativeEquipLookup = const void*(*)(int,int);
// Called only with native recipe records. Installation validates ABI and code.
inline bool filterTreeExclusion(void* context, const void* recipe,
                               TreeExcluded original, NativeEquipLookup lookup) {
 if(recipe && lookup) {
  const auto bytes=static_cast<const unsigned char*>(recipe);
  if(bytes[8]==13) {
   std::uint16_t index=0;
   std::memcpy(&index,bytes+0x1c,sizeof(index));
   const auto equip=static_cast<const unsigned char*>(lookup(13,index));
   if(equip) {
    std::uint16_t id=0;
    std::memcpy(&id,equip+0x48,sizeof(id));
    if(isStormslingerTreeEntry(13,id)) return false;
   }
  }
 }
 return original(context,recipe);
}
}
