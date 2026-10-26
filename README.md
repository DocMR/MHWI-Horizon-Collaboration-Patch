# MHWI Horizon Collaboration Patch — Review Source

Source materials for Nexus Mods manual review of [mod 8884](https://www.nexusmods.com/monsterhunterworld/mods/8884), version 1.0.

## Contents
- `snapshots/native`: source snapshot corresponding to the accepted native DLL.
- `snapshots/stracker`: source snapshot corresponding to the accepted Stracker plugin, including the quest-mapping link fix.
- `delivery-tools`: current unified installer, installation/uninstallation and local feedback-export scripts.
- `BUILD.md`: build requirements and commands.
- `RELEASE-HASHES.json`: submitted release and runtime binary identifiers.
- `FILE-SHA256.json`: integrity inventory of this review tree.
- `UPLOAD-GUIDE-zh-CN.md`: local upload checklist and GitHub Desktop instructions in Chinese.

These are source files for inspection, not an installable game patch. No game models, textures, sound banks, video, save files or compiled patch DLLs are included. Snapshot directory names and relative layout are retained because CMake uses the adjacent staging source directories. Historical development scripts may refer to the author's original workspace; use the root build scripts instead.

## Runtime and tools
The DLLs hook functions inside the game process and map collaboration resources. MinHook is included as a dependency with its original license. The unified installer selects native mode or a supported Stracker plugin mode. Diagnostic export is local and does not automatically submit reports.

## Rights
This review publication does not grant a new blanket license. Original game content and associated intellectual property belong to their respective rights holders. Third-party source remains subject to its included licenses. No game asset redistribution permission is asserted by this repository.
