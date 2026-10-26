#include "horizon_shield_aux_lifecycles.h"
#include "horizon_owner_verification.h"
namespace horizon_other {
namespace {
#include "generated_shield_aux_owners.inc"
}
bool verify_shield_auxiliary_lifecycle_owners(const std::uint8_t* image) noexcept {
    if (!image) return false;
    for (const auto& owner : kShieldAuxiliaryOwners)
        if (!verify_native_owner(image, owner)) return false;
    return true;
}
template class ShieldAuxiliaryLifecycles<64,64>;
}
