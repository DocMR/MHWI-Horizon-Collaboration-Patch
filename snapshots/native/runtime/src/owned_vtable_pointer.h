#pragma once
#include <Windows.h>
#include <cstdint>

namespace stormslinger {
struct VtablePointerApi {
    static BOOL protect(void* address, SIZE_T bytes, DWORD flags, DWORD* old) {
        return VirtualProtect(address, bytes, flags, old);
    }
};
// One aligned pointer, with an explicit quiescent install/restore boundary.
// No shared return-stub code is patched. Failed maintenance keeps ownership;
// callers must retain the replacement's lifetime until restoration succeeds.
template<class Api = VtablePointerApi>
class OwnedVtablePointer {
    void* volatile* address_ = nullptr;
    void* before_ = nullptr;
    void* after_ = nullptr;
    bool owned_ = false, protection_pending_ = false;
    DWORD saved_protection_ = 0;

    bool repair() {
        if (!protection_pending_) return true;
        DWORD unused = 0;
        if (!Api::protect(const_cast<void**>(address_), sizeof(void*), saved_protection_, &unused)) return false;
        protection_pending_ = false;
        return true;
    }
    bool exchange(void* expected, void* replacement) {
        if (!repair()) return false;
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(const_cast<void**>(address_), &memory, sizeof(memory)) || memory.State != MEM_COMMIT ||
            (memory.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        const auto access = memory.Protect & 0xFF;
        const auto writable = (access == PAGE_EXECUTE || access == PAGE_EXECUTE_READ ||
            access == PAGE_EXECUTE_READWRITE || access == PAGE_EXECUTE_WRITECOPY)
            ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
        DWORD old = 0;
        if (!Api::protect(const_cast<void**>(address_), sizeof(void*), writable, &old)) return false;
        saved_protection_ = old;
        protection_pending_ = true;
        const bool matched = InterlockedCompareExchangePointer(address_, replacement, expected) == expected;
        if (matched) owned_ = replacement == after_;
        const bool maintained = repair();
        return matched && maintained;
    }
public:
    OwnedVtablePointer() = default;
    OwnedVtablePointer(const OwnedVtablePointer&) = delete;
    OwnedVtablePointer& operator=(const OwnedVtablePointer&) = delete;
    bool active() const noexcept { return owned_ || protection_pending_; }
    bool install(void* slot, void* expected, void* replacement) {
        if (active() || !slot || (reinterpret_cast<std::uintptr_t>(slot) & (sizeof(void*) - 1)) ||
            !expected || !replacement || expected == replacement) return false;
        void* current = nullptr;
        SIZE_T read = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), slot, &current, sizeof(current), &read) ||
            read != sizeof(current) || current != expected) return false;
        address_ = static_cast<void* volatile*>(slot);
        before_ = expected;
        after_ = replacement;
        return exchange(before_, after_);
    }
    bool restore() { return repair() && (!owned_ || exchange(after_, before_)); }
};
} // namespace stormslinger
