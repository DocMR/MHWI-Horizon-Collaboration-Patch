# Building the runtime DLLs

## Requirements
Windows x64; MSVC C++ compiler, Windows SDK and MASM; CMake and Ninja on PATH. The release was built using the author's Visual Studio 2026 x64 toolchain. MinHook 1.3.4 source is included and does not need to be downloaded.

Clone/download the entire repository, retaining the relative directory structure. Open an **x64 Native Tools Command Prompt for Visual Studio**, change to the repository root, then run:

```bat
build-native.cmd
build-stracker.cmd
```

Targets and output locations:
- Native: `StormslingerStandalone`, output `build/native/dinput8.dll`.
- Stracker: `HorizonCollaboration`, output `build/stracker/HorizonCollaboration.dll`.

The wrappers configure Release builds with the complete collaboration, original quest IDs, native quest saving and weapon tree filtering enabled. The Stracker build also enables STORMSLINGER_HORIZON_STRACKER.

Historical source snapshots are separated intentionally: the native runtime remains the accepted native build; the plugin has a later source/link fix. Do not merge the source snapshots before reviewing the release.

The portable wrappers have been prepared for reviewers; a fresh clone build on another PC has not yet been verified. Compiler/toolchain differences may produce different binary hashes. Historical scripts under each snapshot's tools directory show the original build invocations and self-test targets.

## Installer review
`delivery-tools/Install-Patch.cmd` contains the actual self-contained PowerShell dispatcher used by the release. Its accompanying ZIP is deliberately excluded because it contains game resources. The readable PowerShell sources are included alongside it. Packaging scripts retain workspace paths and are provided for inspection, not as a standalone game-asset download or packaging service.

The source tree alone cannot create the complete installable asset package. Do not run installation/uninstallation scripts against a game directory merely to review the source.
