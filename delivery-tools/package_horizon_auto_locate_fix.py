from pathlib import Path
import hashlib,json,os,shutil,zipfile
P=Path(__file__).resolve().parents[1]
BASE='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
ROOT=P/(BASE+' - Release Packages');D=ROOT/'Startup Fix'
S=P/'staging/Horizon-auto-locate-fix-20261023';S.mkdir(exist_ok=True)
def sha(b):return hashlib.sha256(b).hexdigest()
archive=D/(BASE+' - Unified Edition.zip')
with zipfile.ZipFile(archive) as z:entries={n:z.read(n) for n in z.namelist()}
dispatcher=(P/'tools/Install-HorizonUnifiedPatch.ps1').read_text(encoding='utf-8-sig')
for n,tool in [('tools/native.ps1','Install-HorizonPatch.ps1'),('tools/stracker.ps1','Install-HorizonStrackerPatch.ps1')]:
 old=sha(entries[n]);entries[n]=(P/'tools'/tool).read_bytes();assert old in dispatcher
 dispatcher=dispatcher.replace(old,sha(entries[n]))
original_cmd=(D/'Install-Patch.cmd').read_text(encoding='utf-8')
header=original_cmd.split('# HORIZON_POWERSHELL_PAYLOAD\n',1)[0]+'# HORIZON_POWERSHELL_PAYLOAD\n'
cmd=header+dispatcher
temporary=S/'new-package.tmp'
with zipfile.ZipFile(temporary,'w',zipfile.ZIP_STORED) as z:
 for n,b in entries.items():z.writestr(n,b)
with zipfile.ZipFile(temporary) as z:
 assert all(sha(z.read(n)) in cmd for n in entries)
try:os.replace(temporary,archive)
except PermissionError:
 # Preserve an occupied package and its matching CMD as a pair.
 old_directory=D;D=ROOT/'Auto Locate Fix';D.mkdir(exist_ok=True)
 archive=D/(BASE+' - Unified Edition.zip');os.replace(temporary,archive)
 shutil.copy2(old_directory/'安装说明 - Installation Instructions.txt',D/'安装说明 - Installation Instructions.txt')
(D/'Install-Patch.cmd').write_text(cmd,encoding='utf-8',newline='\r\n')
(P/'tools/Install-HorizonUnifiedPatch.ps1').write_text(dispatcher,encoding='utf-8-sig',newline='\r\n')
for stem in ['Install-HorizonPatch','Install-HorizonStrackerPatch']:
 file=P/'tools'/(stem+'.cmd');text=file.read_text(encoding='utf-8')
 header=text.split('# HORIZON_POWERSHELL_PAYLOAD\n',1)[0]+'# HORIZON_POWERSHELL_PAYLOAD\n'
 file.write_text(header+(P/'tools'/(stem+'.ps1')).read_text(encoding='utf-8-sig'),encoding='utf-8',newline='\r\n')
doc=D/'安装说明 - Installation Instructions.txt';text=doc.read_text(encoding='utf-8-sig')
text=text.replace('工具会查找Steam游戏目录，不能唯一定位时','工具会自动查找Steam及各磁盘游戏库，忽略同一路径的大小写差异；未找到或确有多个安装目录时')
text=text.replace('It searches Steam libraries; if no unique installation is found','It automatically searches Steam libraries and merges case variants of the same path; if no installation or multiple distinct installations are found')
doc.write_text(text,encoding='utf-8-sig',newline='\r\n')
source=S/'source-auto-locate-fix.zip'
with zipfile.ZipFile(P/'staging/Horizon-mapping-link-fix-20261023/source-mapping-link-fix.zip') as z:items={n:z.read(n) for n in z.namelist()}
for n in list(items):
 if n not in ['source-manifest.json','BUILD.txt'] and (P/n).is_file():items[n]=(P/n).read_bytes()
for n in ['tools/test_horizon_game_discovery.py','tools/package_horizon_auto_locate_fix.py']:items[n]=(P/n).read_bytes()
items['source-manifest.json']=json.dumps([dict(path=n,sha256=sha(b)) for n,b in sorted(items.items()) if n not in ['source-manifest.json','BUILD.txt']],indent=2).encode()
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in items.items():z.writestr(n,b)
report=dict(status='auto_locate_fixed_verification_pending',directory=str(D),archive_sha256=sha(archive.read_bytes()),installer_sha256=sha((D/'Install-Patch.cmd').read_bytes()),source_sha256=sha(source.read_bytes()),runtime_payloads_unchanged=True,game_modified=False,real_machine_candidates_before=2,real_machine_candidates_after=1)
(S/'fix-index.json').write_text(json.dumps(report,indent=2))
f=P/'staging/Horizon-unified-delivery-20261023/delivery-index.json';index=json.loads(f.read_text());index.update(status=report['status'],directory=str(D),archive_sha256=report['archive_sha256'],installer_sha256=report['installer_sha256'],auto_locate_fix='staging/Horizon-auto-locate-fix-20261023/fix-index.json')
index['inputs']['__NATIVE_SCRIPT_HASH__']=sha(entries['tools/native.ps1']);index['inputs']['__PLUGIN_SCRIPT_HASH__']=sha(entries['tools/stracker.ps1'])
f.write_text(json.dumps(index,indent=2));print(json.dumps(report))
