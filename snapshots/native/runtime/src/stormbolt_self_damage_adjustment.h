#pragma once

namespace stormslinger {
// PC121EA10: positional MS x64 ABI (RCX, XMM1, XMM2, R9B, stack).
using StormboltHealthAdjustment = void (*)(void*,float,float,bool,bool);
enum class StormboltSelfDamageDispatch { skipped, unavailable, invoked };
// Caller supplies the borrowed actor7658 component after completing the native
// conditional gate and stage8 state writes/notifications. This does not establish
// readiness, object lifetime or whether native health guards permit a change.
inline StormboltSelfDamageDispatch stormbolt_adjust_self_damage(
    void* component,float amount,StormboltHealthAdjustment adjust) {
    if (!(amount>0.0f)) return StormboltSelfDamageDispatch::skipped;
    if (!component || !adjust) return StormboltSelfDamageDispatch::unavailable;
    adjust(component,amount,1.0f,true,false);
    return StormboltSelfDamageDispatch::invoked;
}
} // namespace stormslinger
