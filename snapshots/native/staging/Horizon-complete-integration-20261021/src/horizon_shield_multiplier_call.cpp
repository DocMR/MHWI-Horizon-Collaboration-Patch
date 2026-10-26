#include "horizon_shield_multiplier_call.h"
#include "horizon_owner_verification.h"
extern "C" { float (*horizon_shield_multiplier_dispatch)(void*,float)=nullptr; }
namespace horizon_other {
namespace {
#include "generated_shield_multiplier_owner.inc"
}
bool verify_shield_multiplier_owner(const std::uint8_t* image) noexcept {
    return image && verify_native_owner(image,kShieldMultiplierOwner);
}
template class ShieldMultiplierCall<64>;
}
