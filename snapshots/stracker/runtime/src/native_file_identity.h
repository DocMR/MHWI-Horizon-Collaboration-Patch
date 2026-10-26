#pragma once
#include <Windows.h>
#include <cstdint>

namespace stormslinger {
struct NativeFileIdentity {
    std::uint32_t volume = 0;
    std::uint64_t file_index = 0, size = 0, modified = 0;
    bool operator==(const NativeFileIdentity& other) const noexcept {
        return volume == other.volume && file_index == other.file_index &&
            size == other.size && modified == other.modified;
    }
};
// Caller owns lifetime of borrowed handle. Does not close it or classify ownership.
inline bool query_native_file_identity(HANDLE handle, NativeFileIdentity& output,
    DWORD& error) noexcept {
    error = ERROR_SUCCESS;
    if (!handle || handle == INVALID_HANDLE_VALUE) { error = ERROR_INVALID_HANDLE; return false; }
    BY_HANDLE_FILE_INFORMATION before{}, after{};
    if (!GetFileInformationByHandle(handle, &before)) { error = GetLastError(); return false; }
    if (before.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { error = ERROR_DIRECTORY; return false; }
    if (!GetFileInformationByHandle(handle, &after)) { error = GetLastError(); return false; }
    const auto identity = [](const BY_HANDLE_FILE_INFORMATION& value) noexcept {
        return NativeFileIdentity{value.dwVolumeSerialNumber,
            (static_cast<std::uint64_t>(value.nFileIndexHigh) << 32) | value.nFileIndexLow,
            (static_cast<std::uint64_t>(value.nFileSizeHigh) << 32) | value.nFileSizeLow,
            (static_cast<std::uint64_t>(value.ftLastWriteTime.dwHighDateTime) << 32) | value.ftLastWriteTime.dwLowDateTime};
    };
    const auto candidate = identity(before);
    if (!(candidate == identity(after))) { error = ERROR_RETRY; return false; }
    output = candidate;
    return true;
}
} // namespace stormslinger
