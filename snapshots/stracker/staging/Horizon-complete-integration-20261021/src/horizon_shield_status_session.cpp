#include "horizon_shield_status_session.h"
#include "horizon_owner_verification.h"
namespace horizon_other {
namespace {
#include "generated_shield_lifecycle_owners.inc"
}
bool verify_shield_status_lifecycle_owners(const std::uint8_t* image) noexcept {
    if (!image) return false;
    for (const auto& owner : kShieldLifecycleOwners)
        if (!verify_native_owner(image, owner)) return false;
    return true;
}
template class ShieldStatusSession<64>;
}
