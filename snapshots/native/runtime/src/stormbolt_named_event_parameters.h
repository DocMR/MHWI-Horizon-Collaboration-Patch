#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace stormslinger {
// PC22FEF80 reads flags at80, actor at30; PS flags98 cannot be copied.
// Caller supplies authenticated native default vectors and a live actor.
// This builds parameters only; it neither dispatches nor owns native requests.
struct StormboltNamedEventParameters {
    std::array<std::array<float,4>,3> vectors{};
    void* actor=nullptr;
    std::array<std::uint64_t,9> callbacks{};
    std::uint32_t flags=0;
};
static_assert(offsetof(StormboltNamedEventParameters,actor)==0x30);
static_assert(offsetof(StormboltNamedEventParameters,callbacks)==0x38);
static_assert(offsetof(StormboltNamedEventParameters,flags)==0x80);
static_assert(sizeof(StormboltNamedEventParameters)==0x88);
inline bool build_stormbolt_actor_event_parameters(
    void* actor, const std::array<std::array<float,3>,3>& nativeDefaults,
    StormboltNamedEventParameters& output) noexcept {
    if (!actor) return false;
    StormboltNamedEventParameters candidate{};
    for (std::size_t v=0;v<3;++v)
        for (std::size_t n=0;n<3;++n) candidate.vectors[v][n]=nativeDefaults[v][n];
    candidate.actor=actor;
    candidate.flags=0x10;
    output=candidate;
    return true;
}
// PS timer STOP supplies actor position in the first vector and flags 0x12.
// PC22FF2C6 consumes that vector when bit 1 is set.
inline bool build_stormbolt_stop_event_parameters(
    void* actor, const std::array<std::array<float,3>,3>& nativeDefaults,
    const std::array<float,3>& actorPosition,
    StormboltNamedEventParameters& output) noexcept {
    StormboltNamedEventParameters candidate{};
    if (!build_stormbolt_actor_event_parameters(actor,nativeDefaults,candidate))
        return false;
    for (std::size_t n=0;n<3;++n) candidate.vectors[0][n]=actorPosition[n];
    candidate.flags=0x12;
    output=candidate;
    return true;
}
} // namespace stormslinger
