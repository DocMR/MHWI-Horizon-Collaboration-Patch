"""Finalize documentation for user-operated install/uninstall acceptance."""
from pathlib import Path
import hashlib,json,shutil,zipfile
P=Path(__file__).resolve().parents[1]
D=P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch - Stracker Edition'
S=P/'staging/Horizon-complete-stracker-stage2-20261023'
doc=D/'安装说明 - Installation Instructions.txt'
B=P/'backups/Horizon-stracker-before-manual-handoff-20261023'
B.mkdir(exist_ok=True)
if not (B/doc.name).exists():shutil.copy2(doc,B/doc.name)
cn='''{Simplified Chinese Version}
怪物猎人世界：冰原－零之地平线联动补丁（保留原DLL版）
本包已完成离线检查，等待手动实机验收；请勿作为已验收正式版公开。

一、前置条件
Windows 10/11 64位，已安装并能正常启动《怪物猎人：世界》及冰原的受支持版本。
本版需要已有的受支持Stracker加载器，且loader-config.json中的enablePluginLoader为true。安装器检查版本；不支持时停止，不下载或替换加载器。不需要Python。
原生版与本版不能同时启用。使用原生版的玩家须先卸载并恢复原加载器。
本包不替换根目录dinput8.dll、loader.dll及其配置，也不包含AMD/Steam DLL。其它MOD仍可能有共享资源冲突，不保证全部兼容。

二、安装
1. 关闭游戏。把原游戏目录文件备份到游戏目录外，保持相对路径；chunk文件夹和MonsterHunterWorld.exe无需复制。保留首次安装前备份。
2. 将本版ZIP与Install-Patch.cmd放在同一文件夹，双击CMD。自动查找失败时，选择MonsterHunterWorld.exe所在目录。
3. 确认已备份后选择“是”；“否”或关闭提示会取消。安装器不自动备份、不启动游戏，安装完成后自行启动。
只需复制ZIP与CMD到其它电脑即可运行安装器，但该电脑仍须满足上述前置条件。
成功一键安装后，下次启动会显示追加内容提示。
可手动把ZIP内容直接解压到游戏根目录，不要增加上层文件夹；此方式跳过安装器前置检查，也不重置已确认的提示记录。
ZIP顶层为nativePC、Export-Feedback.cmd、Uninstall-Patch.cmd、StormslingerSupport；插件位于nativePC/plugins/HorizonCollaboration.dll。
安装器会先检查只读、占用和文件／目录冲突；发现阻挡时不写入。若写入开始后因磁盘或权限变化失败，请先排除问题再安装，或从备份恢复，勿直接启动游戏。

三、卸载与恢复
1. 先换回原版装备并保存，正常退出游戏。如需反馈，先按第四节导出。
2. 双击游戏目录的Uninstall-Patch.cmd，确认卸载。需要恢复被覆盖的原文件时选择“是”，再选择安装前的备份目录；所选目录应直接包含原nativePC等文件，而非多套一层。取消目录选择会停止卸载。
3. 工具移除未修改的补丁清单文件，并从所选备份恢复对应原文件；已修改的补丁文件保留并提示。它不创建卸载前补丁备份，不改原加载器，不清理清单外的MOD，不回退存档。
未备份的被覆盖文件无法找回；其它文件需自行恢复。不要整体删除nativePC，它包含原加载器插件及其它MOD。手动卸载时也须按ZIP的具体文件路径逐项清理，再恢复对应原文件。
卸载后获得的装备、素材、套装预设仍在存档内，但联动功能不可用，部分名称可能变为HARDUMMY。不要穿戴这些装备承接任务，否则可能无法加载任务或打开Esc；先换回原版装备。

四、反馈
在游戏目录双击Export-Feedback.cmd，完成后按Win+R输入：
%LOCALAPPDATA%\\Stormslinger\\feedback
把最新Stormslinger-feedback-*.zip发给作者，附操作步骤、预期／实际表现，必要时加截图或录像。无法启动游戏也可导出。
原始日志目录：%LOCALAPPDATA%\\Stormslinger\\logs
默认不收集存档、玩家姓名或崩溃转储。请在卸载前导出。
联系方式：QQ群1131440257；Discord：mr.son0522。

五、启动问题
先卸载并恢复备份，确认原环境可以启动。若已确认是旧修改版amd_ags_x64.dll冲突，备份后换回匹配游戏版本的官方文件；不要从不明来源下载DLL。安装／卸载本包均不会修复或恢复AMD AGS文件。

受支持加载器SHA256（安装器自动核验）：
dinput8.dll：6e38baff0bddc5014046e3ba5a733814f95f65d5ca67e2fb15d18c5106d4e059
loader.dll：17ec93d9d57809e4968666961caf996f7d819c05b280fbb6d444b95920a801ee
若已安装QuestLoader.dll，仅支持：97380a19c12822c318ebc7ef09df601823cbf33ec674e1aee9f8a690d5422c08
'''
en='''{英文版}
Monster Hunter World: Iceborne - Horizon Zero Dawn Collaboration Patch (Stracker Edition)
Offline checks are complete; manual in-game acceptance is pending. Do not publish this as a live-accepted release.

1. Requirements
64-bit Windows 10/11 and a supported, working installation of Monster Hunter: World and Iceborne.
An existing supported Stracker loader is required, with enablePluginLoader set to true in loader-config.json. The installer checks versions and stops on an unsupported version; it does not download or replace loaders. Python is not required.
The native edition and this edition must not be active together. Uninstall the native edition and restore the original loader first.
This package does not replace the root dinput8.dll, loader.dll or loader configuration, and includes no AMD/Steam DLLs. Other mods may still conflict through shared resources; universal mod compatibility is not guaranteed.

2. Installation
1. Close the game. Back up the existing game files outside the game directory, preserving relative paths. The chunk folder and MonsterHunterWorld.exe need not be copied. Keep the original backup from before your first installation.
2. Keep this edition's ZIP and Install-Patch.cmd together, then double-click the CMD. If automatic detection fails, select the folder containing MonsterHunterWorld.exe.
3. Select Yes only after making a backup. No or closing the prompt cancels installation. The installer does not create backups or launch the game. Launch it yourself after installation.
Only the ZIP and CMD need to be copied to another PC to run the installer; that PC must still meet the requirements above.
After successful one-click installation, the added-content notice appears on the next launch.
Alternatively, extract the ZIP directly into the game root without an enclosing folder. This bypasses installer prerequisite checks and does not reset an acknowledged notice.
ZIP root entries are nativePC, Export-Feedback.cmd, Uninstall-Patch.cmd and StormslingerSupport. The plugin is nativePC/plugins/HorizonCollaboration.dll.
The installer checks read-only files, file locks and file/folder conflicts before writing. If blocked, it writes nothing. If disk or permission changes cause a failure after writing starts, resolve the issue and reinstall or restore your backup before launching the game.

3. Uninstallation and Restoration
1. Equip base-game gear, save and exit normally. Export feedback first if needed, using Section 4.
2. Run Uninstall-Patch.cmd in the game directory and confirm. To restore overwritten originals, select Yes and choose the pre-installation backup folder, which should directly contain the original nativePC and other files without an extra enclosing folder. Cancelling folder selection stops uninstallation.
3. The tool removes unchanged files in the patch manifest and restores corresponding originals from the chosen backup. Modified patch files are preserved and reported. It does not create a pre-uninstall patch backup, alter the original loader, remove mods outside its manifest or roll back saves.
Overwritten files cannot be recovered without a backup; restore any other files yourself. Do not delete the entire nativePC folder: it contains original loader plugins and other mods. For manual uninstallation, remove individual files using their exact ZIP paths, then restore corresponding originals.
Earned equipment, materials and loadouts remain in saves after uninstallation, but collaboration features are unavailable and some names may become HARDUMMY. Do not undertake quests with this equipment: loading or the Esc menu may fail. Switch back to base-game gear first.

4. Feedback
Run Export-Feedback.cmd in the game directory. When finished, press Win+R and enter:
%LOCALAPPDATA%\\Stormslinger\\feedback
Send the newest Stormslinger-feedback-*.zip to the author with reproduction steps and expected versus actual results; add screenshots or video when needed. Export also works when the game cannot launch.
Raw logs: %LOCALAPPDATA%\\Stormslinger\\logs
Saves, player names and crash dumps are not collected by default. Export before uninstalling.
Contact: QQ group 1131440257; Discord: mr.son0522.

5. Startup Issues
Uninstall, restore your backup and check that the original environment launches. If an old modified amd_ags_x64.dll is confirmed as the cause, back it up and replace it with the official file matching your game build. Do not download DLLs from unknown sources. Installing or uninstalling this package does not repair or restore AMD AGS files.

Supported loader SHA256 values (checked automatically by the installer):
dinput8.dll: 6e38baff0bddc5014046e3ba5a733814f95f65d5ca67e2fb15d18c5106d4e059
loader.dll: 17ec93d9d57809e4968666961caf996f7d819c05b280fbb6d444b95920a801ee
If QuestLoader.dll is installed, only this hash is supported: 97380a19c12822c318ebc7ef09df601823cbf33ec674e1aee9f8a690d5422c08
'''
doc.write_text(cn+'\n'+'='*60+'\n\n'+en,encoding='utf-8-sig',newline='\r\n')
note='''

### 2026-10-23 验收方式更新：全部打包后由用户手动安装／卸载
用户明确要求完成安装包后自行操作安装卸载，替代第三项由助手备份换装的安排。助手不改游戏目录。保留原DLL版ZIP/CMD及完整双语说明已准备，说明已补充加载器精确支持范围、跨电脑前置条件、手动解压限制及禁止整删含原插件的nativePC。二进制与已通过离线测试包保持不变；待用户手动验收启动、原插件、完整联动回归、反馈、卸载恢复、重复安装。不得把待实机项记作通过。
'''
for name in ['项目进度.md','项目目标.md','新聊天交接.md','保留原DLL版-安装包设计方案.md']:
    with (P/name).open('a',encoding='utf-8') as f:f.write(note)
source=S/'source-stracker-stage2.zip'
with zipfile.ZipFile(source) as z:entries={n:z.read(n) for n in z.namelist()}
name='tools/prepare_horizon_stracker_manual_handoff.py';entries[name]=(P/name).read_bytes()
entries['source-manifest.json']=json.dumps([dict(path=n,sha256=hashlib.sha256(b).hexdigest()) for n,b in sorted(entries.items()) if n not in ['source-manifest.json','BUILD.txt']],indent=2).encode()
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
    for n,b in sorted(entries.items()):z.writestr(n,b)
index=json.loads((S/'candidate-index.json').read_text())
index.update(source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),instructions_sha256=hashlib.sha256(doc.read_bytes()).hexdigest(),acceptance_operator='user_manual_install_uninstall',game_modified=False)
(S/'candidate-index.json').write_text(json.dumps(index,indent=2))
print(json.dumps(index))
