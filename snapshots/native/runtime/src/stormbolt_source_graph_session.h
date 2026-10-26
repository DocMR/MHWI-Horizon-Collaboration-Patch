#pragma once
#include "stormbolt_condition_registry_extension.h"
#include "stormbolt_source_graph_patch.h"

namespace stormslinger {
// New condition is caller-owned and must outlive successful restore. Installation
// and restoration require a quiescent machine; restore before native rebuild/free.
class StormboltSourceGraphSession {
    StormboltConditionRegistryExtension registry_;
    StormboltSourceGraphPatch graph_;
public:
    bool active() const noexcept {return registry_.active() || graph_.active();}
    bool install(StormboltGraphState* rows,std::size_t count,void*** conditions,
        std::uint32_t* condition_count,void* new_condition) {
        if(active() || !registry_.prepare(conditions,condition_count,new_condition)) return false;
        if(!graph_.prepare(rows,count,registry_.prepared_objects(),799,798)) {
            registry_.restore();return false;
        }
        if(!registry_.activate()) {graph_.restore();registry_.restore();return false;}
        if(!graph_.activate()) {graph_.restore();registry_.restore();return false;}
        return true;
    }
    bool restore() noexcept {
        // Every graph reference disappears before its registry object slot.
        return graph_.restore() && registry_.restore();
    }
    ~StormboltSourceGraphSession() {
        if(!restore()) {
            // Preserve both published layers together if caller destroys an
            // unrestored session; never remove the key under a live graph.
            graph_.retain_published();registry_.retain_published();
        }
    }
};
} // namespace stormslinger
