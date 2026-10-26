# MHWI Horizon Collaboration Patch

Horizon Zero Dawn collaboration content for the PC version of Monster Hunter: World: Iceborne.

## Download the automatic installer

**Players: download the automatic installation version from the [Releases page](https://github.com/DocMR/MHWI-Horizon-Collaboration-Patch/releases).**

Open the desired release and download the installation files listed under **Assets**. Keep the installer and its accompanying patch archive together, and follow the included installation instructions. Back up your saves and close the game before installing.

GitHub's automatically generated **Source code (zip)** and **Source code (tar.gz)** archives contain repository source files, not the installable patch.

### 中文下载说明

**玩家请前往 [Releases 发布页面](https://github.com/DocMR/MHWI-Horizon-Collaboration-Patch/releases)下载自动安装版。**

进入所需版本，在 **Assets（附件）** 中下载安装文件。安装器和配套补丁压缩包须放在同一文件夹，按随附说明安装；安装前请备份存档并关闭游戏。

GitHub 自动生成的 **Source code (zip)** 和 **Source code (tar.gz)** 是源码归档，不能当作补丁安装。

## Source code and build instructions

This repository also provides source materials for Nexus Mods manual review of [mod 8884](https://www.nexusmods.com/monsterhunterworld/mods/8884), version 1.0. Browse the [source snapshots](snapshots/) directly on GitHub and see [BUILD.md](BUILD.md) for the build requirements and commands.

## Contents
- `snapshots/native`: source snapshot corresponding to the native release DLL.
- `snapshots/stracker`: source snapshot corresponding to the Stracker release plugin, including the quest-mapping link fix.
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
