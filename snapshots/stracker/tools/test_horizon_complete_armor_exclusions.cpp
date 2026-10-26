#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_armor_exclusions.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
int main() {
 using namespace horizon_other;
 std::size_t count=0;const auto* targets=armor_recipe_identities(&count);CHECK(targets && count==30);
 std::array<unsigned char,32> recipe{};
 for(std::size_t i=0;i<count;++i) {
  recipe.fill(0);recipe[8]=targets[i].category;std::memcpy(recipe.data()+10,&targets[i].serialized_row,2);
  const auto unchanged=recipe;
  CHECK(!horizon_armor_exclusion_result(recipe.data(),true));
  CHECK(!horizon_armor_exclusion_result(recipe.data(),false));CHECK(recipe==unchanged);
 }
 // Exclusion adaptation is restricted to exact category/recipe identities.
 for(std::uint8_t category=0;category<6;++category) {
  recipe.fill(0);recipe[8]=category;const std::uint16_t unrelated=0xffff;std::memcpy(recipe.data()+10,&unrelated,2);
  CHECK(horizon_armor_exclusion_result(recipe.data(),true));
  CHECK(!horizon_armor_exclusion_result(recipe.data(),false));
 }
 CHECK(horizon_armor_exclusion_result(nullptr,true));CHECK(!horizon_armor_exclusion_result(nullptr,false));
 std::cout<<"PASS 30 exact native armor recipe exclusions; unrelated category/row and original false retained; recipe unchanged. Material/story/currency admission not bypassed or exercised.\n";
}
