#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <limits>
#include <new>
#include <vector>

namespace stormslinger {
enum class MemoryReadResult { copied, rejected, unreadable, out_of_memory, unexpected_exception, not_target };
// A successful copy does not establish resource identity, lifetime or consistency.
// The caller supplies a validated size limit and must still hold the resource boundary.
inline MemoryReadResult copy_current_process_memory(std::uintptr_t address, std::size_t bytes,
    std::size_t maximum_bytes, std::vector<unsigned char>& output) noexcept {
    if (!address || !bytes || bytes > maximum_bytes ||
        bytes > std::numeric_limits<std::uintptr_t>::max() - address)
        return MemoryReadResult::rejected;
    try {
        std::vector<unsigned char> copied(bytes);
        SIZE_T read = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
            copied.data(), bytes, &read) || read != bytes) return MemoryReadResult::unreadable;
        output.swap(copied);
        return MemoryReadResult::copied;
    } catch (const std::bad_alloc&) { return MemoryReadResult::out_of_memory; }
    catch (...) { return MemoryReadResult::unexpected_exception; }
}
} // namespace stormslinger
