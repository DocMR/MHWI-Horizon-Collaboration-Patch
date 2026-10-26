#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_source_graph_patch.h"
#include "stormbolt_source_graph_session.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
int main() {
    std::array<StormboltGraphState,120> rows{};
    std::vector<std::vector<StormboltGraphLink>> originals;
    originals.reserve(std::size(source_graph_edits));
    for(const auto& edit:source_graph_edits) {
        originals.emplace_back(edit.expected,edit.expected+edit.count);
        rows[edit.state].lists[edit.secondary]={originals.back().data(),static_cast<std::uint32_t>(edit.count),0xABCD};
    }
    const auto snapshot=rows;
    std::array<void*,800> conditions{};
    conditions[776]=&rows;conditions[799]=&conditions;
    StormboltSourceGraphPatch patch;
    assert(!patch.activate());
    assert(!patch.prepare(rows.data(),120,conditions.data(),800,778));
    assert(!patch.prepare(rows.data(),120,conditions.data(),799,799));
    assert(!patch.prepare(rows.data(),119,conditions.data(),800,799));
    assert(!patch.active());
    auto& first=rows[source_graph_edits[0].state].lists[source_graph_edits[0].secondary];
    first.links[0].result^=1;
    assert(!patch.prepare(rows.data(),120,conditions.data(),800,799));
    assert(!patch.activate());first.links[0].result^=1;
    assert(patch.prepare(rows.data(),120,conditions.data(),800,799));
    ++first.count;assert(!patch.activate());--first.count;
    assert(patch.activate() && patch.active());
    assert(!patch.prepare(rows.data(),120,conditions.data(),800,799));
    std::size_t index=0;
    for(const auto& edit:source_graph_edits) {
        const auto& expected=source_graph_ps_expected[index++];
        const auto& list=rows[edit.state].lists[edit.secondary];
        assert(list.count==expected.count && list.reserved==0xABCD);
        for(std::size_t n=0;n<expected.count;++n) {
            auto link=expected.links[n];
            if(link.group==1 && link.key==-30) link.key=799;
            assert(!std::memcmp(list.links+n,&link,sizeof(link)));
        }
    }
    // A foreign slot writer must not cause partial restore or free live arrays.
    auto* owned=first.links;first.links=nullptr;
    assert(!patch.restore() && patch.active());
    first.links=owned;assert(patch.restore() && !patch.active());
    assert(!std::memcmp(rows.data(),snapshot.data(),sizeof(rows)));
    for(std::size_t n=0;n<std::size(source_graph_edits);++n) {
        const auto& e=source_graph_edits[n];
        assert(!std::memcmp(originals[n].data(),e.expected,e.count*sizeof(StormboltGraphLink)));
    }
    std::array<void*,798> native_registry{};
    native_registry[776]=&rows;native_registry[778]=&native_registry;
    const auto saved_registry=native_registry;
    void** registry_slot=native_registry.data();
    std::uint32_t registry_count=798;
    int new_condition=42;
    StormboltSourceGraphSession session;
    assert(session.install(rows.data(),120,&registry_slot,&registry_count,&new_condition));
    assert(registry_count==799 && registry_slot[798]==&new_condition);
    assert(registry_slot[778]==native_registry[778]);
    assert(!std::memcmp(registry_slot,native_registry.data(),sizeof(native_registry)));
    auto* graph_owned=first.links;first.links=nullptr;
    assert(!session.restore() && session.active());
    assert(registry_count==799 && registry_slot[798]==&new_condition);
    first.links=graph_owned;
    auto* registry_owned=registry_slot;registry_slot=native_registry.data();
    assert(!session.restore() && session.active());
    assert(!std::memcmp(rows.data(),snapshot.data(),sizeof(rows)));
    registry_slot=registry_owned;assert(session.restore() && !session.active());
    assert(registry_count==798 && registry_slot==native_registry.data());
    assert(!std::memcmp(native_registry.data(),saved_registry.data(),sizeof(native_registry)));
    std::puts("PASS132 graph list edits match complete PS lists after symbolic ShootGatling remap; native lists preserved, staged failure, foreign-slot retry and exact restore.");
}
