#pragma once
#include "horizon_shield_equipment_session.h"
namespace horizon_other {
// Host certifies full EXE/build first, before any shared sites are modified.
// Checks endpoint bytes then emits typed addresses. Does not call endpoints,
// inspect live resources/managers, establish quiescence or activate a session.
// Failure leaves output untouched. Resource identity/lifetime and bootstrap
// readiness are separate host obligations, including dynamic material slots.
bool bind_shield_equipment_endpoints(const std::uint8_t* image,
    std::size_t image_bytes, ShieldEquipmentBindings& output) noexcept;
}
