#pragma once
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_attachment_update.h"

namespace stormslinger {
// Selected PS6EC5 ->70B0 branch, not the active cleanup/decay path.
template<std::size_t Capacity>
bool stormbolt_run_inactive_update(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_actor,const StormboltDecayOwnerApi& owner,
    const std::uint32_t* ammo_items,const StormboltMaterialColorApi& material,
    const std::array<float,4>& low_hlsa,const std::array<float,4>& high_hlsa) {
    StormboltStageState state{};
    if(!store.snapshot(token,state)) return false;
    auto* weapon=reinterpret_cast<std::byte*>(token.address);
    std::uint8_t mode{};std::memcpy(&mode,weapon+0x2350,1);
    if(mode) {
        mode=0;std::memcpy(weapon+0x2350,&mode,1);
        std::uint8_t alive{};
        if(initial_actor) std::memcpy(&alive,static_cast<const std::byte*>(initial_actor)+0xC,1);
        std::uint32_t ammo=0;
        if(initial_actor && (alive&0xE)) {
            if(!owner.manager_slot || !*owner.manager_slot || !owner.local_player) return false;
            const auto local=owner.local_player(*owner.manager_slot);
            if(!store.snapshot(token,state)) return false;
            if(local==initial_actor) {
                if(!ammo_items) return false;
                std::uint32_t item{};std::memcpy(&item,weapon+0x27DC,4);
                if(!item) ammo=0xFF;
                else for(std::uint32_t index=0;index<41;++index)
                    if(ammo_items[index]==item) {ammo=index;break;}
            }
        }
        std::memcpy(weapon+0x27C8,&ammo,4);
    }
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
