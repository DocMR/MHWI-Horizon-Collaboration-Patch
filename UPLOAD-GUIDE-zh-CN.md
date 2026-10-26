# GitHub 审核源码上传清单与操作步骤

仓库：https://github.com/DocMR/MHWI-Horizon-Collaboration-Patch

## 当前成果与范围

上传本目录内的全部内容，而不是把本目录再次压缩上传，也不是上传整个 E:\ChatGPT-MHWI 工作区。

目录已经包含展开的两个发行源码快照、构建入口、安装卸载和反馈工具。原生版和 Stracker 版保留不同快照，是因为两份发行 DLL 对应的源码存在差异；审核时不能随意合并成一份。

本次仅静态检查和整理，未运行编译、安装、卸载或游戏。BUILD.md 中已明确：在另一台电脑上的全新克隆编译尚未验证，不能把文件完整性核对称为编译成功。

## 需要上传的全部内容

| 路径 | 内容与作用 |
| --- | --- |
| README.md | 英文审核入口、目录说明与使用边界 |
| BUILD.md | 英文编译环境、命令、目标 DLL 和输出位置 |
| build-native.cmd | 原生 dinput8.dll 构建入口 |
| build-stracker.cmd | Stracker 插件 HorizonCollaboration.dll 构建入口 |
| snapshots/native/ | 原生发行源码，保留 runtime、staging、tools 及 BUILD.txt 全部相对路径 |
| snapshots/stracker/ | 插件发行源码，保留 runtime、staging、tools 及 BUILD.txt 全部相对路径 |
| delivery-tools/ | 安装器、卸载器、反馈导出、统一分发及打包脚本，共15个文件 |
| RELEASE-HASHES.json | 原生 DLL、插件 DLL、正式统一归档及安装入口的 SHA-256 |
| FILE-SHA256.json | 本目录逐文件完整性清单，不包含其自身 |
| .gitignore | 排除编译产物与本地临时目录 |
| UPLOAD-GUIDE-zh-CN.md | 本操作说明 |

两个 runtime/vendor/minhook-1.3.4 目录连同原许可证必须保留。生成的 .h、.inc、.asm 也是编译输入，不能当作缓存删掉。

## 不放入源码仓库的内容

- Horizon-Collaboration-Review-Source.zip：不能继续作为审核人员查看源码的唯一入口；展开源码后，旧ZIP可保留作历史附件，也可在确认无需保留后删除，无须重写历史。
- 游戏 EXE、PS/PC 原始资源包、模型、贴图、音频、录像、存档、反馈日志、备份目录、编译输出和更新诊断候选。
- 玩家安装包放在 Releases 的 Assets 中。本目录仅是审核源码，不是可安装补丁。
- 不要把当前工作区的 runtime 或最新诊断候选替换进已匹配发行版本的 snapshots。

## 已完成的静态核对

整理前原有1722个文件共15,768,958字节；原完整性清单1721条全部匹配，没有未列入的文件。

原生快照 CMake 显式文件输入88项、插件快照89项，均存在。此检查不等同于全部传递依赖或编译验证。

RELEASE-HASHES.json 中两份 DLL 与当前手动安装目录内 DLL 一致；安装入口与正式目录 Install-Patch.cmd 一致；正式长文件名统一ZIP也匹配。短文件名 Unified Edition.zip 是另一层归档，不能用其哈希替代长文件名归档哈希。

## 推荐操作：GitHub Desktop 上传到现有仓库

1. 安装并登录 GitHub Desktop，使用有仓库写权限的 DocMR 账号。
2. 点击 File → Clone repository → URL，输入上方仓库地址。选择一个新的本地目录，例如 E:\ChatGPT-MHWI\github-publish\MHWI-Horizon-Collaboration-Patch。不要把整个研究工作区初始化为这个公开仓库。
3. 克隆完成后，点击 Repository → Show in Explorer，打开克隆目录。将本上传目录内部的所有文件和子目录复制到克隆目录根部，保留隐藏的 .gitignore。不要再套一层“GitHub Upload - Horizon Collaboration Patch”文件夹，也不要修改克隆产生的 .git 目录。
4. 如有同名 README.md，先保留原说明中仍需使用的下载介绍，再与本目录审核说明整合。至少保留 BUILD.md、snapshots、delivery-tools 的入口；不要把旧安装说明误当作源码编译说明。
5. 回到 GitHub Desktop，检查 Changes。应直接出现 snapshots/native/runtime/src 下的 .cpp/.h 等文件和 BUILD.md。不要将凭据、游戏文件、安装包或诊断候选纳入本次提交。
6. Summary 填写 `Publish expanded review source and build instructions`，点击 Commit to main（实际默认分支名称以页面为准），再点击 Push origin。不要使用 Force push。
7. 打开GitHub网页，确认仓库为 Public；Code页直接看得到 snapshots、delivery-tools、BUILD.md，并且可以点开 .cpp 文件阅读。复制本次提交的完整提交链接。
8. 需要固定审核版本时，在此次源码提交上创建新的说明性Tag，例如 review-source-1.0；不要覆盖已有版本Tag。玩家安装包继续保留在已有 Releases 中。
9. 回复审核团队，提供下面四个入口；提交链接必须替换成实际上传后生成的链接。

## 发给审核人员的链接

- 仓库：https://github.com/DocMR/MHWI-Horizon-Collaboration-Patch
- 编译说明：在仓库Code页点开 BUILD.md，复制实际文件链接（无需猜测分支名）。
- 源码：在仓库Code页点开 snapshots，复制实际目录链接。
- 固定版本：复制本次 Commit 或 Tag 链接，说明该源码对应版本1.0、两种加载方式及 RELEASE-HASHES.json 中的发行文件。

英文回复示例：

> Hello, I have now committed and pushed the expanded source files to the public repository, so they can be browsed directly on GitHub without downloading a ZIP. BUILD.md documents the runtime build requirements, commands and output files. The repository includes separate source snapshots for the native DLL and the Stracker plugin, as well as the installer, uninstaller and feedback-export scripts. RELEASE-HASHES.json identifies the corresponding release binaries. The runtime source can be reviewed independently; game assets are not included in the source repository. A fresh-clone build on another PC has not yet been verified.
>
> Repository: [insert repository URL]
>
> Build instructions: [insert BUILD.md URL]
>
> Source snapshot / commit: [insert commit URL]

## 上传后维护

如果只修改README、编译说明或本说明，重新生成 FILE-SHA256.json 即可；不要改写 RELEASE-HASHES.json 中现有二进制哈希。以后发布新二进制时，应同时提交对应源码、更新发行文件映射并建立新版本记录。

GitHub Desktop 官方克隆说明：https://docs.github.com/en/desktop/adding-and-cloning-repositories/cloning-and-forking-repositories-from-github-desktop
