#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <array>
#include <vector>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_quest_completion_fact.h"
#include "../staging/Horizon-complete-integration-20261021/src/horizon_native_integration.h"
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_mapping_transaction.h"
namespace {
struct MappingReference {
 std::uint32_t rva;std::uint8_t size,displacement_offset;
 std::uint32_t table_index;std::uint8_t expected[7];
};
#include "../staging/Horizon-complete-integration-20261021/src/generated_quest_mapping_references.inc"
}
#define CHECK(x) do { if(!(x)) {std::cerr<<"failed line "<<__LINE__<<"\n";return 1;} } while(0)
int main() {
 using namespace horizon_other;
 auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,0x55D3000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 CHECK(image);
 const auto candidate=quest_mapping_candidate();
 std::memcpy(image+0x324FAC0,candidate.entries,candidate.original_count*4);
 const std::int32_t sentinel=-1;std::memcpy(image+0x324FAC0+candidate.original_count*4,&sentinel,4);
 for(const auto& r:kMappingReferences)std::memcpy(image+r.rva,r.expected,r.size);
 std::ifstream input("research/horizon-complete-tutorial-owner-fixture.bin",std::ios::binary);
 CHECK(input.read(reinterpret_cast<char*>(image+0x1F3A870),985));
 PreparedQuestMapping mapping;
 CHECK(mapping.prepare(image).status==MappingRedirectStatus::prepared);
 CHECK(!quest_mapping_references_active(mapping));
 QuestMappingTransaction borrowed;
 CHECK(borrowed.install_while_quiescent(image,mapping,false)==MappingInstallResult::installed);
 CHECK(image[0x1F3A965]==0x90);
 for(const auto& r:kMappingReferences)CHECK(!std::memcmp(image+r.rva,r.expected,r.size));
 // Root publishes its redirects. The borrower must never restore those bytes.
 for(std::size_t i=0;i<mapping.redirect_count();++i) {
  const auto& edit=mapping.redirects()[i];std::memcpy(image+edit.displacement_rva,edit.after,4);
 }
 CHECK(quest_mapping_references_active(mapping));
 std::vector<std::uint8_t> character(0x104c20+64);
 constexpr std::int32_t quests[]={67101,67102,67607,67802,67803,67804};
 auto set_bit=[&](std::size_t field,std::size_t index) {
  std::uint32_t word=0;std::memcpy(&word,character.data()+field+(index/32)*4,4);
  word|=std::uint32_t(1)<<(index%32);std::memcpy(character.data()+field+(index/32)*4,&word,4);
 };
 for(std::size_t i=0;i<6;++i) {
  CHECK(candidate.entries[352+i]==quests[i]);
  CHECK(read_source_quest_open(character.data(),quests[i],mapping)==QuestOpenFact::closed);
  set_bit(0x104bd8,352+i);
  CHECK(read_source_quest_open(character.data(),quests[i],mapping)==QuestOpenFact::open);
 }
 CHECK(read_source_required_clear(character.data(),67804,mapping)==QuestClearFact::not_cleared);
 set_bit(0x104c20,357); // Own completion is not the Survivor prerequisite.
 CHECK(read_source_required_clear(character.data(),67804,mapping)==QuestClearFact::not_cleared);
 set_bit(0x104c20,356);
 CHECK(read_source_required_clear(character.data(),67804,mapping)==QuestClearFact::cleared);
 CHECK(source_quest_open_requirements(67804,999,24,QuestClearFact::unknown)==QuestRequirementResult::clear_unknown);
 CHECK(source_quest_open_requirements(67804,999,24,QuestClearFact::not_cleared)==QuestRequirementResult::prerequisite_unmet);
 CHECK(source_quest_open_requirements(67804,999,24,QuestClearFact::cleared)==QuestRequirementResult::requirements_met);
 struct Floor {int id,hr,mr;};
 for(const auto f:{Floor{67101,11,0},Floor{67102,6,0},Floor{67607,50,0},Floor{67802,0,24},Floor{67803,0,24},Floor{67804,0,24}}) {
  CHECK(source_quest_rank_floor(f.id,f.hr,f.mr)==RankFloorResult::met);
  CHECK(source_quest_rank_floor(f.id,f.hr ? f.hr-1:0,f.mr ? f.mr-1:0)==RankFloorResult::unmet);
  CHECK(source_quest_rank_floor(f.id,-1,0)==RankFloorResult::invalid_rank);
 }
 const auto before_read=character;
 CHECK(read_source_quest_open(character.data(),999999,mapping)==QuestOpenFact::unknown);
 CHECK(character==before_read);
 CHECK(borrowed.restore_while_quiescent());
 CHECK(image[0x1F3A965]==0x74);
 CHECK(quest_mapping_references_active(mapping));
 for(std::size_t i=0;i<mapping.redirect_count();++i) {
  const auto& edit=mapping.redirects()[i];std::memcpy(image+edit.displacement_rva,edit.before,4);
 }
 QuestMappingTransaction owned;
 CHECK(owned.install_while_quiescent(image,mapping)==MappingInstallResult::installed);
 CHECK(quest_mapping_references_active(mapping));
 CHECK(owned.restore_while_quiescent());
 CHECK(!quest_mapping_references_active(mapping));
 CHECK(read_source_quest_open(character.data(),67804,mapping)==QuestOpenFact::unknown);
 CHECK(read_source_required_clear(character.data(),67804,mapping)==QuestClearFact::unknown);
 for(const auto& r:kMappingReferences)CHECK(!std::memcmp(image+r.rva,r.expected,r.size));
 image[0x1F3A870]^=1;
 QuestMappingTransaction mismatch;
 CHECK(mismatch.install_while_quiescent(image,mapping,false)==MappingInstallResult::original_mismatch);
 CHECK(!mismatch.active());
 for(const auto& r:kMappingReferences)CHECK(!std::memcmp(image+r.rva,r.expected,r.size));
 // One composed clone must include every bow and Stormslinger edit, preserving
 // all other instructions. A bad Stormslinger site must leave bow bytes intact.
 for (int which=0;which<2;++which) {
  const std::uint32_t rva=which ? 0x13738b0 : 0x13732f0;
  const std::size_t bytes=which ? 121 : 798;
  std::array<std::uint8_t,798> original{},clone{};
  std::ifstream tree(which ? "research/horizon-complete-tree-predicate-fixture.bin" : "research/horizon-complete-tree-builder-fixture.bin",std::ios::binary);
  CHECK(tree.read(reinterpret_cast<char*>(original.data()),bytes));clone=original;
  CHECK(compose_complete_weapon_tree_edits(rva,clone.data(),bytes)==TreeEditResult::applied);
  for(std::size_t i=0;i<bytes;++i) {
   const bool edited=which ? ((i>=50 && i<52)||(i>=60 && i<62)||(i>=103 && i<105))
    : ((i>=358 && i<364)||(i>=367 && i<373)||(i>=414 && i<420));
   CHECK(clone[i]==(edited ? 0x90 : original[i]));
  }
  CHECK(compose_complete_weapon_tree_edits(rva,clone.data(),bytes)==TreeEditResult::unchanged);
  clone=original;clone[which ? 103 : 414]^=1;const auto corrupt=clone;
  CHECK(compose_complete_weapon_tree_edits(rva,clone.data(),bytes)==TreeEditResult::conflict);
  CHECK(clone==corrupt);
 }
 CHECK(VirtualFree(image,0,MEM_RELEASE));
 std::cout<<"PASS borrowed tutorial-only ownership, root redirects preserved, owned rollback, digest rejection; composed bow+Stormslinger clones and conflict preservation; six rank floors/open bits and prerequisite clear facts\n";
}
