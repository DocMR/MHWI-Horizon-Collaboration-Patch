#ifdef NDEBUG
#undef NDEBUG
#endif
#include "owned_vtable_pointer.h"
#include <cassert>
#include <cstdio>

using namespace stormslinger;
struct FaultPointerApi : VtablePointerApi {
    inline static int attempts = 0, fail_at = 0;
    static BOOL protect(void* p, SIZE_T n, DWORD flags, DWORD* old) {
        if (++attempts == fail_at) return FALSE;
        return VtablePointerApi::protect(p, n, flags, old);
    }
};
int main() {
    auto** page = static_cast<void**>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(page);
    int before = 0, after = 0, foreign = 0;
    page[0] = &before;
    DWORD old = 0;
    assert(VirtualProtect(page, 4096, PAGE_READONLY, &old));
    OwnedVtablePointer<FaultPointerApi> patch;
    assert(!patch.install(reinterpret_cast<unsigned char*>(page) + 1, &before, &after));
    assert(!patch.install(page, &foreign, &after));
    FaultPointerApi::fail_at = 2;
    assert(!patch.install(page, &before, &after) && patch.active() && page[0] == &after);
    FaultPointerApi::fail_at = 0;
    assert(patch.restore() && !patch.active() && page[0] == &before);
    MEMORY_BASIC_INFORMATION info{};
    assert(VirtualQuery(page, &info, sizeof(info)) && info.Protect == PAGE_READONLY);
    assert(patch.install(page, &before, &after));
    FaultPointerApi::fail_at = FaultPointerApi::attempts + 2;
    assert(!patch.restore() && patch.active() && page[0] == &before);
    FaultPointerApi::fail_at = 0;
    assert(patch.restore() && !patch.active());
    assert(patch.install(page, &before, &after));
    assert(VirtualProtect(page, 4096, PAGE_READWRITE, &old));
    InterlockedExchangePointer(page, &foreign);
    assert(VirtualProtect(page, 4096, old, &old));
    assert(!patch.restore() && patch.active() && page[0] == &foreign);
    assert(VirtualQuery(page, &info, sizeof(info)) && info.Protect == PAGE_READONLY);
    std::puts("PASS: aligned pointer ownership, failed protection recovery, exact restore, "
              "foreign writer retained without forced overwrite");
    // Deliberately retain the owned synthetic page until process exit on conflict.
}
