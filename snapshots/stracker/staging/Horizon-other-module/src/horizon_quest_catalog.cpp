#include "horizon_quest_catalog.h"

namespace horizon_other {
namespace {
constexpr QuestDescriptor catalog[] = {
#include "generated_quest_catalog.inc"
};
}

const QuestDescriptor* quest_catalog(std::size_t& count) noexcept {
    count = sizeof(catalog) / sizeof(catalog[0]);
    return catalog;
}

const QuestDescriptor* find_quest(std::int32_t id) noexcept {
    for (const auto& quest : catalog) {
        if (quest.id == id) return &quest;
    }
    return nullptr;
}
}
