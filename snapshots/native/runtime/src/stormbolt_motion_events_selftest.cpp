#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_motion_events.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
template<class T> T read(const void* pointer,std::size_t offset=0) {
    T value{};std::memcpy(&value,static_cast<const std::byte*>(pointer)+offset,sizeof(value));return value;
}
int main() {
    StormboltMotionEvents events;
    const auto begin=reinterpret_cast<std::uintptr_t>(&events),end=begin+sizeof(events);
    auto inside=[&](const void* p,std::uint64_t size) {
        const auto a=reinterpret_cast<std::uintptr_t>(p);
        return a>=begin && a<=end && size<=end-a;
    };
    unsigned trees=0,empty=0,properties=0;
    bool found_flag=false;
    for(const auto& definition:stormbolt_motion_event_roots) {
        const void* root=nullptr;assert(events.select(definition.slot,root));
        if(!root) {++empty;continue;}
        ++trees;assert(inside(root,48));
        const auto* groups=read<const std::byte*>(root);
        const auto group_count=read<std::uint64_t>(root,8);
        assert(group_count<100 && inside(groups,group_count*24));
        for(std::uint64_t g=0;g<group_count;++g) {
            const auto* group=groups+g*24;
            const auto* props=read<const std::byte*>(group);
            const auto count=read<std::uint64_t>(group,8);
            assert(count<100 && inside(props,count*24));
            for(std::uint64_t p=0;p<count;++p) {
                const auto* prop=props+p*24;
                const auto* keys=read<const std::byte*>(prop);
                const auto n=read<std::uint64_t>(prop,8);
                assert(n<10000 && (!n || inside(keys,n*20)));
                ++properties;
                if(definition.slot==244 && read<std::uint32_t>(group,16)==0x48D6114Fu &&
                   read<std::uint32_t>(prop,16)==0x08FD20A6u) {
                    assert(n==1 && read<std::uint32_t>(keys)==6 && read<std::uint32_t>(keys,4)==6);
                    assert(read<std::uint16_t>(keys,16)==5);
                    found_flag=true;
                }
            }
        }
    }
    const void* unchanged=&events;
    assert(!events.select(232,unchanged) && unchanged==&events);
    assert(!events.select(245,unchanged) && unchanged==&events);
    assert(trees==5 && empty==8 && properties>0 && found_flag);
    std::printf("PASS:13 source selections (5 trees/8 explicitly empty), %u properties relocated inside stable owner; unrelated slots unchanged.\n",properties);
}
