#include "horizon_shield_native_bindings.h"
#include "horizon_owner_verification.h"
#include "../../../runtime/src/stormbolt_power_cell_sync_codec.h"
#include <limits>
namespace horizon_other {
namespace {
#include "generated_shield_binding_owners.inc"
}
bool bind_shield_equipment_endpoints(const std::uint8_t* image,
    std::size_t image_bytes, ShieldEquipmentBindings& output) noexcept {
    const auto base=reinterpret_cast<std::uintptr_t>(image);
    if (!image || image_bytes!=0x55D3000 ||
        base>(std::numeric_limits<std::uintptr_t>::max)()-image_bytes) return false;
    for (const auto& owner:kShieldBindingOwners)
        if (!verify_native_owner(image,owner)) return false;
    ShieldEquipmentBindings b{};
    const stormslinger::StormboltLoopInputBindings inputs{
        base+0x324B1F0,base+0x322C268,base+0x5011F68,base+0x31E0078,
        0}; // Equipment skill applies with any weapon; no bowgun-only gate.
    const auto query=reinterpret_cast<stormslinger::StormboltSkillValueQuery>(base+0x1922190);
    const auto prompt=reinterpret_cast<stormslinger::StormboltSkillPrompt>(base+0x19A2D20);
    const auto actor_flag=reinterpret_cast<decltype(b.status.actor_flag)>(base+0x1F604B0);
    auto* manager=reinterpret_cast<void**>(base+0x500ECA0);
    const auto local_player=reinterpret_cast<decltype(b.remote.local_player)>(base+0x1B42010);
    const auto equipment=reinterpret_cast<stormslinger::StormboltEquipmentEffectRequest>(base+0xCCC620);
    b.status={inputs,query,actor_flag,base+0x500ECA0};
    b.timer={inputs,query,prompt,reinterpret_cast<decltype(b.timer.native_query)>(base+0x1F628A0)};
    b.loop.audio={base+0x324A830,reinterpret_cast<decltype(b.loop.audio.container_at)>(base+0x130C710),
        reinterpret_cast<const float*>(base+0x503C430),reinterpret_cast<const float*>(base+0x3FDE890),
        inputs,query,reinterpret_cast<stormslinger::StormboltNamedEventRequest>(base+0x2300260)};
    b.loop.material_at=reinterpret_cast<decltype(b.loop.material_at)>(base+0x222F910);
    b.loop.convert=reinterpret_cast<decltype(b.loop.convert)>(base+0x2171490);
    b.loop.scalar=reinterpret_cast<decltype(b.loop.scalar)>(base+0x23D8180);
    b.loop.material.write_vector=reinterpret_cast<decltype(b.loop.material.write_vector)>(base+0x23D82A0);
    b.growth={{base+0x3240FB0,b.loop.audio,prompt,manager,local_player},base+0x322BBB0,equipment};
    b.sync={base+0x322BBB0,inputs,query,&stormslinger::encode_stormbolt_power_cell_sync};
    b.remote={inputs,b.loop.audio,manager,local_player,equipment,&stormslinger::decode_stormbolt_power_cell_sync};
    output=b;
    return true;
}
}
