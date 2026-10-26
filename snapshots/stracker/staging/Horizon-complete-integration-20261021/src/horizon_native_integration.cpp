#include "horizon_native_integration.h"
#include "horizon_armor_exclusions.h"
#include "horizon_palico_recipe_edits.h"
#include <cstring>

namespace horizon_other {
namespace {
#include "generated_armor_predicate_owner.inc"
}
TreeEditResult compose_complete_weapon_tree_edits(std::uint32_t rva,
    std::uint8_t* clone,std::size_t bytes) noexcept {
    const auto* owner=bow_tree_owner(rva);
    if (!owner) return TreeEditResult::unsupported_owner;
    if (!clone || bytes!=owner->bytes) return TreeEditResult::invalid_buffer;
    const std::uint8_t builder[]={0x0f,0x86,0x46,0x01,0x00,0x00};
    const std::uint8_t predicate[]={0x76,0x08};
    const auto offset=rva==0x13732F0 ? std::size_t(0x19e) : std::size_t(0x67);
    const auto count=rva==0x13732F0 ? sizeof(builder) : sizeof(predicate);
    const auto* expected=rva==0x13732F0 ? builder : predicate;
    if (offset>bytes || count>bytes-offset) return TreeEditResult::invalid_buffer;
    bool applied=true;
    for (std::size_t i=0;i<count;++i) applied=applied && clone[offset+i]==0x90;
    if (!applied && std::memcmp(clone+offset,expected,count)) return TreeEditResult::conflict;
    const auto bow=compose_bow_tree_edits(rva,clone,bytes);
    if (bow!=TreeEditResult::applied && bow!=TreeEditResult::unchanged) return bow;
    std::memset(clone+offset,0x90,count);
    return !applied || bow==TreeEditResult::applied ? TreeEditResult::applied : TreeEditResult::unchanged;
}
NativeIntegrationPlan native_equipment_integration_plan() noexcept {
    return {
        1, sizeof(NativeIntegrationPlan),
        {
            {bow_tree_owner(0x13732F0), &compose_complete_weapon_tree_edits, "bow tree child exclusions"},
            {bow_tree_owner(0x13738B0), &compose_complete_weapon_tree_edits, "bow tree exclusion predicate"},
            {&palico_recipe_owner(), &compose_palico_recipe_edits, "palico recipe exclusions"}
        },
        {&kArmorPredicateOwner, &horizon_armor_exclusion_result}
    };
}
}
