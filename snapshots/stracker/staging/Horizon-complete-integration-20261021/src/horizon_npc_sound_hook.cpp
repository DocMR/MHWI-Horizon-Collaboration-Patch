#include "horizon_npc_sound_hook.h"
#include "horizon_owner_verification.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
#include <cstring>
namespace horizon_other {
namespace {
#include "generated_npc_sound_owners.inc"
const std::uint8_t* image_base=nullptr;
void* original_path=nullptr;
bool attempted=false;
using PathBuilder=void (*)(char*,std::int32_t);
using SelectedEntry=const std::uint8_t* (*)(void*,std::uint32_t,std::uint32_t);
using EquipmentRow=const std::uint8_t* (*)(std::uint32_t,std::int32_t);
using Append=void (*)(char*,const char*,std::size_t);
using Format=void (*)(char*,const char*,std::uint32_t,std::uint32_t);
void npc_sound_path(char* output,std::int32_t kind) {
    std::uint32_t group=0,slot=0,npc=0;
    if (kind>=98 && kind<=100) {
        group=3;slot=static_cast<std::uint32_t>(kind-98);
        npc=static_cast<std::uint32_t>(kind+103);
    }
    else if (kind>=89 && kind<=97) {
        const auto index=static_cast<std::uint32_t>(kind-89);
        group=index/3;slot=index%3;npc=static_cast<std::uint32_t>(kind+112);
    } else {
        reinterpret_cast<PathBuilder>(original_path)(output,kind);
        return;
    }
    // These two original branches format the base first, then read the
    // selected head once. A post-call re-read could mix two selections.
    reinterpret_cast<Format>(const_cast<std::uint8_t*>(image_base)+0x24C380)(
        output,"npc\\npc%03d\\sound\\npc%03d",npc,npc);
    const auto* entry=reinterpret_cast<SelectedEntry>(
        const_cast<std::uint8_t*>(image_base)+0x5F9D40)(nullptr,group,slot);
    if (!entry) return;
    std::uint32_t mode=0;std::int32_t equipment=-1;
    std::memcpy(&mode,entry+0x11C,sizeof(mode));
    std::memcpy(&equipment,entry+0x128,sizeof(equipment));
    if (mode!=1 || equipment==-1) return;
    const auto* row=reinterpret_cast<EquipmentRow>(
        const_cast<std::uint8_t*>(image_base)+0x191A6C0)(0,equipment);
    auto model=equipment;
    if (row) std::memcpy(&model,row+0x1C,sizeof(model));
    const char* suffix=nullptr;
    switch (model) {
    case 35:suffix="_ot035";break;
    case 36:suffix="_ot036";break;
    case 38:suffix="_ot038";break;
    case 73:suffix="_ot073";break;
    case 81:suffix="_ot081";break;
    default:return;
    }
    std::size_t length=0;
    while (length<80 && output[length]) ++length;
    if (length+6>=80) return;
    reinterpret_cast<Append>(const_cast<std::uint8_t*>(image_base)+0x2C468C0)(
        output,suffix,80-length);
}
}
EquipmentQueueResult queue_npc_sound_hook(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image) noexcept {
    if (!image || !transaction.can_add_hooks()) return EquipmentQueueResult::invalid_host;
    if (attempted) return EquipmentQueueResult::already_attempted;
    for (const auto& owner:kNpcSoundOwners)
        if (!verify_native_owner(image,owner)) return EquipmentQueueResult::owner_mismatch;
    attempted=true;image_base=image;
    return transaction.add(const_cast<std::uint8_t*>(image)+0x18E7B50,
        reinterpret_cast<void*>(&npc_sound_path),&original_path)==MH_OK
        ? EquipmentQueueResult::queued : EquipmentQueueResult::hook_failed;
}
}
