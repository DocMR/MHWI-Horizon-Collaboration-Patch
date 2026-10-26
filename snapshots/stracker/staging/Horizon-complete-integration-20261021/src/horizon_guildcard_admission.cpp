#include "horizon_guildcard_admission.h"
#include "horizon_owner_verification.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../../runtime/src/hooks.h"
#include <cstring>
namespace horizon_other {
namespace {
#include "generated_guildcard_admission_owners.inc"
const std::uint8_t* image_base=nullptr;
void* original_title=nullptr;
void* original_pose=nullptr;
void* original_condition=nullptr;
void* original_pose_open=nullptr;
const PreparedQuestMapping* mapping_state=nullptr;
bool attempted=false;
using Condition=bool (*)(const std::uint8_t*,std::int32_t);
using Admission=bool (*)(std::uint32_t);
using Entry=const std::uint8_t* (*)(const void*,std::uint32_t);
template<class T> T read(const std::uint8_t* address) noexcept {
    T value{};std::memcpy(&value,address,sizeof(value));return value;
}
const std::uint8_t* owned_entry(std::uint32_t index,bool pose) {
    const auto id=index+1;
    if (pose ? id!=13 : !((id>=782 && id<=786)||(id>=1500 && id<=1504))) return nullptr;
    const auto resource=read<const void*>(image_base+(pose?0x5012A40:0x5012A28));
    if (!resource) return nullptr;
    const auto* entry=reinterpret_cast<Entry>(const_cast<std::uint8_t*>(image_base)+0x462700)(resource,index);
    if (!entry) return nullptr;
    // Serialized GCOD fields are reordered by original61F280.
    return read<std::uint32_t>(entry+0x14)==id &&
        read<std::uint32_t>(entry+8)==12 &&
        read<std::uint32_t>(entry+0xC)==(id>=1500?10u:1u) &&
        read<std::uint32_t>(entry+0x10)==0 &&
        // PC list resources normalize only these eleven platform fields to0.
        // Accept retained PS2 and adapted PC0; identity/condition/group checks
        // above still bind the exact owned row. Other flags remain excluded.
        (read<std::uint32_t>(entry+0x18)==2 || read<std::uint32_t>(entry+0x18)==0) ? entry : nullptr;
}
bool registered_group(std::uint32_t group) {
    if (!mapping_state || !quest_mapping_references_active(*mapping_state)) return false;
    // Same active resource array and body layout consumed by native1976240.
    // Unlike that star getter, this must not use its numeric-ID fallback.
    const auto* manager=read<const std::uint8_t*>(image_base+0x500ED30);
    if (!manager) return false;
    const auto count=read<std::uint64_t>(manager+0xCEA8);
    // The inline pointer regionAEA8..CEA8 contains1024 native entries.
    if (count>1024) return false;
    for (std::uint64_t n=0;n<count;++n) {
        const auto* resource=read<const std::uint8_t*>(manager+0xAEA8+n*8);
        if (!resource) continue;
        const auto id=read<std::int32_t>(resource+0xE0); // native30F150 body+B0, ID+30
        if (id==67803 || id==67804) return true;
        if (group==1 && (id==67101 || id==67102 || id==67607)) return true;
    }
    return false;
}
bool condition(const std::uint8_t* entry,std::int32_t filter) {
    if (entry) {
        const auto id=read<std::uint32_t>(entry+0x14);
        const bool pose=id==13;
        const bool title=(id>=782 && id<=786)||(id>=1500 && id<=1504);
        if ((pose || title) && owned_entry(id-1,pose)==entry) {
            // Retain original type-filter behavior. Only these exact owned
            // condition12 rows replace the withdrawn online activity calendar.
            if (filter!=-1 && filter!=12) return false;
            return registered_group(read<std::uint32_t>(entry+0xC));
        }
    }
    return reinterpret_cast<Condition>(original_condition)(entry,filter);
}
bool title_admission(std::uint32_t index) {
    return reinterpret_cast<Admission>(original_title)(index) || owned_entry(index,false)!=nullptr;
}
bool pose_admission(std::uint32_t index) {
    return reinterpret_cast<Admission>(original_pose)(index) || owned_entry(index,true)!=nullptr;
}
bool pose_open(const std::uint8_t* character,std::uint32_t index) {
    // The PC getter excludes index12 before consulting its native open bitmap.
    // Restore that query only for our exact source-backed pose13 entry.
    if (index==12 && character && owned_entry(index,true))
        return (read<std::uint32_t>(character+0x16DE68) & (1u<<12))!=0;
    using Open=bool (*)(const std::uint8_t*,std::uint32_t);
    return reinterpret_cast<Open>(original_pose_open)(character,index);
}
}
EquipmentQueueResult queue_guildcard_admission_hooks(stormslinger::HookTransaction& transaction,
    const std::uint8_t* image,const PreparedQuestMapping& mapping) noexcept {
    if (!image || mapping.image()!=image || !mapping.table() || !transaction.can_add_hooks()) return EquipmentQueueResult::invalid_host;
    if (attempted) return EquipmentQueueResult::already_attempted;
    for (const auto& owner:kGuildcardAdmissionOwners)
        if (!verify_native_owner(image,owner)) return EquipmentQueueResult::owner_mismatch;
    attempted=true;image_base=image;mapping_state=&mapping;
    if (transaction.add(const_cast<std::uint8_t*>(image)+0xEE6390,
            reinterpret_cast<void*>(&title_admission),&original_title)!=MH_OK ||
        transaction.add(const_cast<std::uint8_t*>(image)+0xEE6310,
            reinterpret_cast<void*>(&pose_admission),&original_pose)!=MH_OK ||
        transaction.add(const_cast<std::uint8_t*>(image)+0xEE27A0,
            reinterpret_cast<void*>(&condition),&original_condition)!=MH_OK ||
        transaction.add(const_cast<std::uint8_t*>(image)+0x134C120,
            reinterpret_cast<void*>(&pose_open),&original_pose_open)!=MH_OK)
        return EquipmentQueueResult::hook_failed;
    return EquipmentQueueResult::queued;
}
}
