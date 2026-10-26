#include "horizon_shield_reaction_call.h"
#include "horizon_owner_verification.h"
extern "C" { bool (*horizon_shield_reaction_dispatch)(void*)=nullptr; }
namespace horizon_other {
namespace {
#include "generated_shield_reaction_owner.inc"
}
bool verify_shield_reaction_owner(const std::uint8_t* image) noexcept {
    return image && verify_native_owner(image,kShieldReactionOwner);
}
template class ShieldReactionCall<64>;
}
