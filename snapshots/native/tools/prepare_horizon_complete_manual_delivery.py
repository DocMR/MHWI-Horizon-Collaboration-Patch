"""Prepare the accepted C10 manual-install package; do not modify the game."""
from pathlib import Path
import hashlib,json,shutil,zipfile
from stormslinger_release_manager import verify
from horizon_complete_test_install import support_files
P=Path(__file__).resolve().parents[1]
NAME='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
OUT=P/'staging/Horizon-complete-manual-delivery-20261022'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    assert not OUT.exists()
    base=P/'staging/Horizon-complete-cumulative-c10-20261022/package'
    manifest,_,_=verify(base)
    package=OUT/'package';shutil.copytree(base,package)
    shutil.copy2(P/'tools/Uninstall-HorizonPatch.ps1',package/'support-root/StormslingerSupport/Uninstall-Patch.ps1')
    for row in manifest['auxiliary_files']:
        path=package/'support-root'/row['relative_path']
        row.update(size=path.stat().st_size,sha256=sha(path))
    manifest.update(status='accepted_gameplay_pending_manual_delivery_acceptance',
        live_acceptance='User accepted all gameplay and C10 notice checks; actual final archive install/uninstall/reinstall acceptance pending.',
        remaining_project_items=['Actual final-archive installation/uninstallation/reinstallation with player backup restore'],
        first_entry_notice='C10 six pages, compact left layout and native idle restoration; user accepted')
    raw=(json.dumps(manifest,indent=2)+'\n').encode()
    (package/'release-manifest.json').write_bytes(raw)
    (package/'support-root/StormslingerSupport/release-manifest.json').write_bytes(raw)
    verify(package);aux=support_files(package,manifest)
    public=P/NAME
    backup=P/'backups/Stormslinger-public-before-complete-20261022'
    assert public.is_dir() and not backup.exists()
    shutil.copytree(public,backup)
    assert all(sha(p)==sha(backup/p.relative_to(public)) for p in public.rglob('*') if p.is_file())
    archive=public/(NAME+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for root in [package/'payload',package/'support-root']:
            for p in sorted(root.rglob('*')):
                if p.is_file():z.write(p,p.relative_to(root).as_posix())
    expected={r['relative_path']:r['sha256'] for r in manifest['files']+aux}
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist())==set(expected)
        assert all(hashlib.sha256(z.read(n)).hexdigest()==v for n,v in expected.items())
    shutil.copy2(P/'tools/Install-HorizonPatch.cmd',public/'Install-Patch.cmd')
    instructions='安装说明 - Installation Instructions.txt'
    text=(backup/instructions).read_text(encoding='utf-8-sig')
    text=text.replace('怪物猎人世界：冰原－零之地平线联动补丁（原生版1.0.0）{Simplified Chinese Version}',
        '{Simplified Chinese Version}\n怪物猎人世界：冰原－零之地平线联动补丁（原生完整版）')
    text=text.replace('Monster Hunter World: Iceborne - Horizon Zero Dawn Collaboration Patch (Native Version 1.0.0) {英文版}',
        '{英文版}\nMonster Hunter World: Iceborne - Horizon Zero Dawn Collaboration Patch (Complete Native Edition)')
    text=text.replace('2. 打开压缩包，将全部内容直接解压到MonsterHunterWorld.exe所在目录。',
        '2. 将ZIP和Install-Patch.cmd放在一起（仅需这两个文件），双击Install-Patch.cmd。工具自动查找Steam游戏目录；不能唯一定位时，请选择MonsterHunterWorld.exe所在目录。\n3. 双语弹窗中确认已备份后选择“是”才解压；选择“否”或关闭弹窗则取消。工具不自动备份，也不启动游戏。也可将ZIP全部内容手动解压到游戏根目录。')
    text=text.replace('卸载工具自动备份的补丁在：%LOCALAPPDATA%\\Stormslinger\\uninstall-backups',
        '本版卸载不再备份卸载前的补丁。卸载时选择你自己安装前保存的原文件备份目录（其中直接包含nativePC、dinput8.dll等），工具恢复被覆盖的同名文件；已修改的补丁文件保留并提示。选择目录时取消会停止卸载。')
    text=text.replace('2. Open the archive and extract all contents directly into the folder containing MonsterHunterWorld.exe.',
        '2. Keep the ZIP and self-contained Install-Patch.cmd together (only these two files are required). Double-click Install-Patch.cmd. It searches Steam libraries; if no unique installation is found, select the folder containing MonsterHunterWorld.exe.\n3. Select Yes in the bilingual prompt only after backing up. No or closing the prompt cancels extraction. The tool does not create a backup or start the game. Manual extraction of all ZIP contents directly into the game root is also supported.')
    text=text.replace('Direct extraction cannot automatically preserve files before they are overwritten.',
        'No pre-uninstall copy of the patch is created. Select your own pre-installation backup folder containing nativePC, dinput8.dll and other original files directly. Matching overwritten files are restored, including any backed-up support files. Modified patch files remain preserved. Cancelling the folder picker stops uninstallation.\nDirect extraction cannot automatically preserve files before they are overwritten.')
    (public/instructions).write_text(text,encoding='utf-8-sig',newline='\r\n')
    # Preserve the paired production sources and add the exact delivery tools.
    source=OUT/'source-complete-native.zip'
    with zipfile.ZipFile(P/'staging/Horizon-complete-cumulative-c10-20261022/source-c10.zip') as old:
        sources={n:old.read(n) for n in old.namelist() if n not in ['source-manifest.json','BUILD.txt']}
    for rel in ['tools/Uninstall-HorizonPatch.ps1','tools/Install-HorizonPatch.ps1','tools/Install-HorizonPatch.cmd',
                'tools/prepare_horizon_complete_manual_delivery.py']:
        sources[rel]=(P/rel).read_bytes()
    with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
        for n,b in sorted(sources.items()):z.writestr(n,b)
        z.writestr('source-manifest.json',json.dumps([{'path':n,'sha256':hashlib.sha256(b).hexdigest()} for n,b in sorted(sources.items())],indent=2))
        z.writestr('BUILD.txt','Accepted C10 runtime. Run tools/build-horizon-complete-c10-runtime-20261022.cmd in x64 MSVC. Delivery tools updated for backup restoration.\n')
    report=dict(status='ready_for_manual_install_uninstall_acceptance',folder=str(public),
        archive_sha256=sha(archive),archive_files=len(expected),dll_sha256=sha(package/'payload/dinput8.dll'),
        source_sha256=sha(source),old_public_backup=str(backup),game_modified=False)
    (OUT/'delivery-index.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report))

if __name__=='__main__':main()
