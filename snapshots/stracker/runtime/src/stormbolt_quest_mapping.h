#pragma once
#include "hooks.h"
#if defined(STORMSLINGER_HORIZON_COMPLETE)
#include "../../staging/Horizon-complete-integration-20261021/src/horizon_quest_mapping.h"
#else
#include "../../staging/Horizon-other-module/src/horizon_quest_mapping.h"
#endif
#include <cstring>
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
#include "quest_loader_mapping_compatibility.h"
#endif

namespace stormslinger {
namespace quest_mapping_detail {
struct MappingReference {
 std::uint32_t rva;
 std::uint8_t size,displacement_offset;
 std::uint32_t table_index;
 std::uint8_t expected[7];
};
// Shared authenticated native references; no tutorial-menu edit is imported.
#include "../../staging/Horizon-other-module/src/generated_quest_mapping_references.inc"
}
class StormboltQuestMapping {
 horizon_other::PreparedQuestMapping prepared_;
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
 QuestLoaderMappingCompatibility compatible_;
#endif
public:
 bool prepare(const unsigned char* image) {
  if(prepared_.table()) return prepared_.image()==image;
  return prepared_.prepare(image).status==horizon_other::MappingRedirectStatus::prepared;
 }
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
 bool prepare(const unsigned char* image,std::uintptr_t loader) {
  if(compatible_.prepared())return compatible_.prepared_for(image);
  if(prepare(image))return true;
  if(!compatible_.prepare(image,loader,quest_mapping_detail::kMappingReferences,horizon_other::quest_mapping_candidate()))return false;
#if defined(STORMSLINGER_HORIZON_STRACKER)
  return prepared_.bind_authenticated_shared_view(image,compatible_.table(),compatible_.reference_count(),
      [](const void* context) noexcept {return static_cast<const QuestLoaderMappingCompatibility*>(context)->active();},&compatible_);
#else
  return true;
#endif
 }
#endif
 MH_STATUS queue(HookTransaction& transaction) {
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
  if(compatible_.prepared())return compatible_.queue(transaction);
#endif
  using namespace quest_mapping_detail;
  constexpr auto count=sizeof(kMappingReferences)/sizeof(kMappingReferences[0]);
  if(!prepared_.table() || prepared_.redirect_count()!=count || !transaction.can_add_hooks())
   return MH_ERROR_NOT_INITIALIZED;
  for(std::size_t i=0;i<count;++i) {
   const auto& reference=kMappingReferences[i];
   const auto& edit=prepared_.redirects()[i];
   if(edit.displacement_rva!=reference.rva+reference.displacement_offset)
    return MH_ERROR_UNSUPPORTED_FUNCTION;
   INT32 displacement=0;std::memcpy(&displacement,edit.after,4);
   auto status=transaction.add_rip_patch(
       const_cast<unsigned char*>(prepared_.image())+reference.rva,
       reference.expected,reference.size,reference.displacement_offset,displacement);
   if(status!=MH_OK)return status;
  }
  return MH_OK;
 }
 const horizon_other::PreparedQuestMapping& prepared() const noexcept{return prepared_;}
 bool references_active() const {
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
  if(compatible_.prepared())return compatible_.active();
#endif
  return horizon_other::quest_mapping_references_active(prepared_);
 }
};
}
