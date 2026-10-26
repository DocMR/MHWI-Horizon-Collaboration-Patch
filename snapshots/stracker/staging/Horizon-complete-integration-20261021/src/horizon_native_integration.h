#pragma once
#include "horizon_bow_tree_edits.h"

namespace horizon_other {
using CloneComposer = TreeEditResult (*)(std::uint32_t, std::uint8_t*, std::size_t) noexcept;
using ArmorResultFilter = bool (*)(const void*, bool) noexcept;
using OriginalArmorPredicate = bool (*)(void* native_manager, const void* native_recipe);
struct CloneEditBinding {
    const TreeOwnerSpec* owner;
    CloneComposer compose;
    const char* purpose;
};
struct PostCallBinding {
    const TreeOwnerSpec* owner;
    ArmorResultFilter filter;
};
struct NativeIntegrationPlan {
    std::uint32_t abi_version;
    std::size_t structure_bytes;
    CloneEditBinding clone_edits[3];
    PostCallBinding armor_post_call;
};
// Compose bow and Stormslinger exclusions in one original owner. Only the
// native exclusion branches are removed; crafting qualification/cost stays native.
TreeEditResult compose_complete_weapon_tree_edits(std::uint32_t owner_rva,
    std::uint8_t* clone,std::size_t bytes) noexcept;
// Metadata/callback access only. The host must certify original image/owner
// SHA256, compose all module edits BEFORE relocation, and install each owner
// once before equipment population. Armor post-call invokes the ORIGINAL
// predicate first, retaining the borrowed recipe pointer across that call,
// then applies only its exclusion result filter. No host action
// is performed here; full quests/save/special mechanics remain separate work.
NativeIntegrationPlan native_equipment_integration_plan() noexcept;
}
