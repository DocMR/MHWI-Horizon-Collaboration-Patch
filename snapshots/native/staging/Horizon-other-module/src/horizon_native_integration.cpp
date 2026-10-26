#include "horizon_native_integration.h"
#include "horizon_armor_exclusions.h"
#include "horizon_palico_recipe_edits.h"

namespace horizon_other {
namespace {
#include "generated_armor_predicate_owner.inc"
}
NativeIntegrationPlan native_equipment_integration_plan() noexcept {
    return {
        1, sizeof(NativeIntegrationPlan),
        {
            {bow_tree_owner(0x13732F0), &compose_bow_tree_edits, "bow tree child exclusions"},
            {bow_tree_owner(0x13738B0), &compose_bow_tree_edits, "bow tree exclusion predicate"},
            {&palico_recipe_owner(), &compose_palico_recipe_edits, "palico recipe exclusions"}
        },
        {&kArmorPredicateOwner, &horizon_armor_exclusion_result}
    };
}
}
