"""Prepare a private pre-notice delivery preview; never installs or publishes."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile
import argparse
from stormslinger_release_manager import verify
from horizon_complete_test_install import support_files

P = Path(__file__).resolve().parents[1]
BASE = P/'staging/Horizon-complete-tutorial-payload-20261021-c7'
OLD = P/'staging/Horizon-complete-task-test-c1/package'
OUT = P/'staging/Horizon-complete-delivery-preview-20261021'
NAME = 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
DLL = P/'runtime/ninja-horizon-complete-c1-20261021/dinput8.dll'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(with_notice=False,output=None):
    global OUT,DLL
    if with_notice:
        OUT=P/'staging/Horizon-complete-cumulative-c2-20261021'
        DLL=P/'runtime/ninja-horizon-complete-c2-notice-20261021/dinput8.dll'
    if output:
        OUT=(P/output).resolve()
        assert OUT.is_relative_to(P/'staging'),'Output must stay inside project staging'
    if OUT.exists():
        raise ValueError('Preserve existing delivery preview')
    evidence = json.loads((P/'research/horizon-complete-presentation-readiness-20261021.json').read_text(encoding='utf-8-sig'))
    for row in evidence['files']:
        assert sha(BASE/'nativePC'/row['path']) == row['sha256']
    old, _, _ = verify(OLD)
    if with_notice:
        proof=json.loads((P/'research/horizon-complete-notice-native-candidate-20261021.json').read_text(encoding='utf-8'))
        assert sha(DLL)==proof['hashes'][DLL.relative_to(P).as_posix()]
    else:
        assert sha(DLL) == old['standalone_dll_sha256']
    for name in ['horizon-complete-extract-installer-tests-20261021.json', 'horizon-complete-uninstall-no-backup-20261021.json']:
        assert json.loads((P/'research'/name).read_text(encoding='utf-8-sig'))['status'].endswith('passed')
    package = OUT/'package'
    (package/'payload').mkdir(parents=True)
    shutil.copytree(BASE/'nativePC', package/'payload/nativePC')
    shutil.copy2(DLL, package/'payload/dinput8.dll')
    shutil.copytree(OLD/'support-root', package/'support-root')
    shutil.copy2(P/'tools/Uninstall-HorizonPatch.ps1', package/'support-root/StormslingerSupport/Uninstall-Patch.ps1')
    if with_notice:
        shutil.copy2(P/'tools/Export-Feedback.ps1',package/'support-root/StormslingerSupport/Export-Feedback.ps1')
    manifest = dict(old)
    manifest.update(status='private_preview_notice_missing_not_for_live_acceptance',
                    candidate_resources='c7', full_project_complete=False,
                    live_acceptance='Not started; all implementation including the new entry notice must finish first',
                    remaining_project_items=['User final entry-notice copy and native14-language implementation',
                                             'Unified live acceptance and final public packaging'],
                    uninstall_predelete_patch_backup=False,
                    automatic_extract_entry='External Install-Patch.cmd beside the archive',
                    files=[{'relative_path': f.relative_to(package/'payload').as_posix(),
                            'size': f.stat().st_size, 'sha256': sha(f)}
                           for f in sorted((package/'payload').rglob('*')) if f.is_file()])
    manifest['auxiliary_files'] = [{'relative_path': row['relative_path'],
                                   'size': (package/'support-root'/row['relative_path']).stat().st_size,
                                   'sha256': sha(package/'support-root'/row['relative_path'])}
                                  for row in old['auxiliary_files']]
    manifest['first_entry_notice'] = 'Not implemented; awaiting user final text'
    if with_notice:
        manifest.update(status='private_cumulative_candidate_pending_unified_live_acceptance',
                        standalone_dll_sha256=sha(DLL),runtime_build='horizon-complete-20261021-c2-notice',
                        first_entry_notice='Implemented: before title,6 pages,14 game languages; live acceptance pending',
                        remaining_project_items=['Unified live acceptance of all content and new tools',
                                                 'Resolve acceptance findings and publish final paired package/source'])
        manifest['diagnostics']=dict(manifest.get('diagnostics',{}),version='1.1.0-candidate',build='horizon-complete-20261021-c2-notice')
    raw = (json.dumps(manifest, indent=2)+'\n').encode('utf-8')
    (package/'release-manifest.json').write_bytes(raw)
    (package/'support-root/StormslingerSupport/release-manifest.json').write_bytes(raw)
    verify(package)
    support_files(package, manifest)
    distribution = OUT/'distribution'/NAME
    (distribution/'StormslingerInstaller').mkdir(parents=True)
    archive = distribution/(NAME+'.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for root in [package/'payload', package/'support-root']:
            for file in sorted(root.rglob('*')):
                if file.is_file():
                    z.write(file, file.relative_to(root).as_posix())
    shutil.copy2(P/'tools/Install-HorizonPatch.cmd', distribution/'Install-Patch.cmd')
    shutil.copy2(P/'tools/Install-HorizonPatch.ps1', distribution/'StormslingerInstaller/Install-Patch.ps1')
    instruction = '''{Simplified Chinese Version}
怪物猎人世界：冰原－零之地平线联动补丁（内部预览，尚未交付）
本预览缺少待审定的首次入游提示，不用于实测验收。

前置：Windows10/11 64位、可正常运行的世界及冰原、补丁支持的游戏构建。
工具使用系统Windows PowerShell/.NET，不需要Python或额外MOD加载器。

安装
1. 关闭游戏。先手动将原游戏根目录文件备份到游戏目录外，保留目录结构；大型原版chunk及EXE可不复制。尤其保留原nativePC、dinput8.dll及其它可能同名的文件。
2. 保持ZIP、Install-Patch.cmd和StormslingerInstaller文件夹放在一起，双击Install-Patch.cmd。它会查找Steam游戏目录；不能唯一定位时请选MonsterHunterWorld.exe所在目录。
3. 双语弹窗中确认已经备份后选择是，才开始解压；选择否或关闭则取消。不会自动备份或启动游戏。
也可以手动将ZIP全部内容直接解压到游戏根目录。nativePC和dinput8.dll应直接位于该目录，不套上层文件夹。

卸载
关闭游戏，双击游戏目录的Uninstall-Patch.cmd。工具不再备份卸载前的补丁文件。
仅清理清单中且hash未变的补丁文件；修改文件、其它MOD及存档保留。需要恢复被覆盖原文件时，按提示选择自己安装前的备份目录。
手动卸载须按清单移除本补丁并恢复自己的原文件备份。混装其它MOD时不要直接删除整个nativePC。

反馈
双击游戏目录Export-Feedback.cmd，打开%LOCALAPPDATA%\\Stormslinger\\feedback，将最新反馈ZIP连同复现步骤、预期/实际结果及必要录像发给作者。
日志有容量限制；反馈包需手动导出，不自动上传，不包含存档。卸载前先导出反馈。
本包不保证外部MOD/加载器兼容；不替换AMD AGS或游戏主程序。

============================================================
{英文版}
Monster Hunter World: Iceborne - Horizon Zero Dawn Collaboration Patch
PRIVATE PREVIEW, NOT A FINAL RELEASE
The first-entry notice is awaiting final copy. This preview is not for live acceptance.

Requirements: 64-bit Windows10/11, working World and Iceborne installations, and the supported game build.
Tools use Windows PowerShell/.NET. Python and additional mod loaders are not required.

Installation
1. Close the game. Manually back up the existing game-root files outside the game directory, preserving their folder structure. Large original chunk files and the EXE may be excluded. Preserve existing nativePC, dinput8.dll and any other files the patch may overwrite.
2. Keep the ZIP, Install-Patch.cmd and StormslingerInstaller folder together. Double-click Install-Patch.cmd. It searches Steam libraries; if no unique installation is found, select the directory containing MonsterHunterWorld.exe.
3. Choose Yes in the bilingual prompt only after making your backup. No or closing the prompt cancels extraction. No automatic backup is created and the game is not started.
Manual installation is also supported: extract all ZIP contents directly into the game root. nativePC and dinput8.dll must be directly inside it, without an enclosing folder.

Uninstallation
Close the game and run Uninstall-Patch.cmd in the game root. Patch files are no longer backed up before removal.
Only listed, unchanged patch files are removed. Modified files, unrelated mods and saves are kept. To restore overwritten originals, select your own pre-installation backup when prompted.
For manual removal, remove the patch-listed files and restore your originals. Do not delete the whole nativePC folder when other mods are installed.

Feedback
Run Export-Feedback.cmd from the game root. Open %LOCALAPPDATA%\\Stormslinger\\feedback and send the newest feedback ZIP with reproduction steps, expected/actual behavior and a video if needed.
Logs have size limits. Feedback is exported manually, never uploaded automatically, and excludes saves. Export feedback before uninstalling.
Compatibility with external mods/loaders is not guaranteed. This package does not replace AMD AGS or the game executable.
'''
    if with_notice:
        instruction=instruction.replace('本预览缺少待审定的首次入游提示，不用于实测验收。',
            '内部累计测试候选，尚非公开成品。首次提示在标题界面出现前按游戏语言分6页显示，确认最后一页后同一内容版本不再重复。')
        instruction=instruction.replace('The first-entry notice is awaiting final copy. This preview is not for live acceptance.',
            'Private cumulative test candidate, not a public release. A six-page notice is shown before the title screen in the game language. Confirming the last page records this content version for the local user.')
    (distribution/'安装说明 - Installation Instructions.txt').write_bytes(instruction.replace('\n','\r\n').encode('utf-8-sig'))
    # Pair runtime sources with the exact current native DLL. Retain the two
    # module source paths referenced by CMake, but exclude builds/player data.
    sources = []
    for folder in ['runtime/src','runtime/vendor','staging/Horizon-complete-integration-20261021/src','staging/Horizon-other-module/src']:
        sources.extend(f for f in (P/folder).rglob('*') if f.is_file() and not any(part in {'.git','__pycache__','build'} for part in f.relative_to(P/folder).parts))
    sources.extend((P/'tools').glob('*.cpp'))
    sources.extend([P/'runtime/CMakeLists.txt', Path(__file__).resolve(), P/'tools/Install-HorizonPatch.cmd',
                    P/'tools/Install-HorizonPatch.ps1', P/'tools/Uninstall-HorizonPatch.ps1',
                    P/'tools/Export-Feedback.ps1', P/'tools/horizon_complete_test_install.py'])
    sources = sorted(set(sources))
    source_zip = OUT/('source-notice-c2.zip' if with_notice else 'source-pre-notice.zip')
    with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED) as z:
        for file in sources:
            z.write(file,file.relative_to(P).as_posix())
        z.writestr('BUILD.txt', ('Private cumulative C2 source snapshot. ' if with_notice else 'Private pre-notice source snapshot. ')+'Runtime DLL SHA256: '+sha(DLL)+'\n'
                   'From an x64 MSVC developer environment with CMake/Ninja, run:\n'
                   'cmake -S runtime -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSTORMSLINGER_ORIGINAL_QUEST_IDS=ON -DSTORMSLINGER_NATIVE_QUEST_SAVE=ON -DSTORMSLINGER_WEAPON_TREE_FILTER=ON -DSTORMSLINGER_HORIZON_COMPLETE=ON\n'
                   'cmake --build build --target StormslingerStandalone\n'
                   'Resources are the paired package payload; source regeneration from PS archives is not required for this runtime build.\n'
                   +('Six-page before-title notice is implemented; unified live acceptance is pending.\n' if with_notice else 'New first-entry notice is not yet implemented. Do not label this source/package final.\n'))
        z.writestr('source-manifest.json',json.dumps([{'path': f.relative_to(P).as_posix(), 'sha256':sha(f)} for f in sources],indent=2))
    with zipfile.ZipFile(archive) as z:
        expected = {r['relative_path']:r['sha256'] for r in manifest['files']+manifest['auxiliary_files']}
        expected['StormslingerSupport/release-manifest.json'] = hashlib.sha256(raw).hexdigest()
        assert set(z.namelist()) == set(expected)
        assert all(hashlib.sha256(z.read(name)).hexdigest() == digest for name,digest in expected.items())
        assert not any(name.endswith('.txt') and 'LICENSE' not in name for name in z.namelist())
    result = {'status':'private_delivery_preview_prepared_notice_missing_no_install_or_publication',
              'package':package.relative_to(P).as_posix(), 'payload_files':len(manifest['files']),
              'archive_files':len(expected), 'archive':archive.relative_to(P).as_posix(), 'archive_sha256':sha(archive),
              'source_archive':source_zip.relative_to(P).as_posix(), 'source_sha256':sha(source_zip), 'source_files':len(sources),
              'uninstall_predelete_patch_backup':False, 'external_one_click_extract_entry':True,
              'first_entry_notice_implemented':with_notice, 'public_stormslinger_unchanged':True,
              'real_game_modified':False,'real_saves_modified':False}
    if with_notice:result['status']='private_cumulative_c2_prepared_no_install_no_live_acceptance'
    (OUT/'preview-index.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    report=OUT.name.replace('Horizon','horizon',1)+'.json' if with_notice else 'horizon-complete-delivery-preview-20261021.json'
    (P/'research'/report).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))


if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--with-notice',action='store_true')
    parser.add_argument('--output')
    args=parser.parse_args()
    main(args.with_notice,args.output)
