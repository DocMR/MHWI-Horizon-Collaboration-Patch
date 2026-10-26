#include "horizon_tutorial_trigger.h"
#include "horizon_owner_verification.h"
#include <cstring>

extern "C" {
std::uintptr_t horizon_tutorial_character_slot = 0;
void horizon_tutorial_trigger_entry();
}

namespace {
const std::uint8_t* tutorial_image = nullptr;

template<class T> T read(const std::uint8_t* address) {
    T value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}
}

extern "C" void horizon_tutorial_request_if_due() {
    const auto* image = tutorial_image;
    if (!image) return;
    using Quantity = std::int32_t (*)(std::int32_t, std::int32_t);
    using Character = const std::uint8_t* (*)(const void*);
    using Request = bool (*)(void*, std::int32_t, bool);
    // PS mode2 uses this storage scope, not the combined selector2 query.
    if (reinterpret_cast<Quantity>(const_cast<std::uint8_t*>(image)+0x1896DF0)(2708, 1) == 0)
        return;
    const auto* manager = read<const void*>(image+0x5013950);
    const auto* character = reinterpret_cast<Character>(
        const_cast<std::uint8_t*>(image)+0x1B8DBB0)(manager);
    // Original item-record bitmap index2707; never synthesize this bit.
    if (!(read<std::uint8_t>(character+0xF6EEA) & 0x08)) return;
    auto* gui = read<void*>(image+0x51C4640);
    // Native producer retains its read-status check, callback and page group.
    (void)reinterpret_cast<Request>(const_cast<std::uint8_t*>(image)+0x1AE1AF0)(gui, 123, false);
}

namespace horizon_other {
namespace {
#include "generated_tutorial_trigger_owners.inc"
}

bool TutorialTriggerCall::prepare_while_quiescent(const std::uint8_t* image,
    stormslinger::HookTransaction& transaction) {
    if (attempted_ || !image || !transaction.can_add_hooks() || tutorial_image ||
        horizon_tutorial_character_slot) return false;
    attempted_ = true;
    for (const auto& owner : kTutorialTriggerOwners)
        if (!verify_native_owner(image, owner)) return false;
    image_ = image;
    tutorial_image = image;
    horizon_tutorial_character_slot = reinterpret_cast<std::uintptr_t>(image+0x5013950);
    bindings_published_ = true;
    call_.emplace(&transaction);
    return call_->prepare(const_cast<std::uint8_t*>(image)+0x1F32167,
        reinterpret_cast<std::uintptr_t>(&horizon_tutorial_trigger_entry),
        std::array<unsigned char,7>{0x48,0x8B,0x0D,0xE2,0x17,0x0E,0x03});
}

bool TutorialTriggerCall::activate_after_shared_commit_while_quiescent() {
    return bindings_published_ && call_ && call_->activate();
}

bool TutorialTriggerCall::restore_while_quiescent() {
    if (call_ && !call_->restore()) return false;
    if (bindings_published_) {
        if (tutorial_image != image_ || horizon_tutorial_character_slot !=
            reinterpret_cast<std::uintptr_t>(image_+0x5013950)) return false;
        tutorial_image = nullptr;
        horizon_tutorial_character_slot = 0;
        bindings_published_ = false;
    }
    return true;
}

bool TutorialTriggerCall::release_after_shared_stop_while_quiescent() {
    if (bindings_published_) return false;
    return !call_ || call_->release_after_quiescence();
}
}
