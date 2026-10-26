#include "weapon_tree_filter_adapter.h"
#include <array>
#include <cstdio>
#include <cstdlib>
static std::array<unsigned char,0x50> equip{};
static void* expectedContext=nullptr;
static const void* expectedRecipe=nullptr;
static int delegated=0,lookups=0;
static bool sentinel=true,missing=false;
static bool original(void* c,const void* r) {
 if(c!=expectedContext || r!=expectedRecipe) std::abort();
 ++delegated;return sentinel;
}
static const void* lookup(int category,int index) {
 if(category!=13 || index!=230) std::abort();
 ++lookups;return missing?nullptr:equip.data();
}
int main() {
 std::array<unsigned char,0x34> recipe{};recipe[8]=13;
 std::uint16_t index=230;std::memcpy(recipe.data()+0x1c,&index,2);
 expectedContext=&recipe;expectedRecipe=recipe.data();
 for(unsigned id=0;id<65536;++id) {
  std::uint16_t word=static_cast<std::uint16_t>(id);std::memcpy(equip.data()+0x48,&word,2);
  for(bool value:{false,true}) {
   sentinel=value;int before=delegated;
   bool result=stormslinger::filterTreeExclusion(expectedContext,expectedRecipe,original,lookup);
   bool target=id==238 || id==239;
   if(result!=(target?false:value) || delegated-before!=(target?0:1))return 1;
  }
 }
 missing=true;sentinel=true;
 if(!stormslinger::filterTreeExclusion(expectedContext,expectedRecipe,original,lookup))return 2;
 expectedRecipe=nullptr;
 if(!stormslinger::filterTreeExclusion(expectedContext,nullptr,original,lookup))return 3;
 expectedRecipe=recipe.data();recipe[8]=11;int before=lookups;
 if(!stormslinger::filterTreeExclusion(expectedContext,expectedRecipe,original,lookup) || lookups!=before)return 4;
 std::puts("PASS: target-only override, original result/arguments preserved, null and lookup failure delegated");
}
