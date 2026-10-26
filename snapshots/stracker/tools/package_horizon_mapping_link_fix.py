from pathlib import Path
import hashlib,io,json,zipfile,shutil
P=Path(__file__).resolve().parents[1]
BASE='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
ROOT=P/(BASE+' - Release Packages')
D=ROOT/'Startup Fix'
S=P/'staging/Horizon-mapping-link-fix-20261023'
D.mkdir(exist_ok=True);S.mkdir(exist_ok=True)
def sha(b):return hashlib.sha256(b).hexdigest()
old=ROOT/'Startup Diagnostic'
with zipfile.ZipFile(old/(BASE+' - Unified Edition.zip')) as z:outer={n:z.read(n) for n in z.namelist()}
before=sha(outer['payload/stracker.zip'])
with zipfile.ZipFile(io.BytesIO(outer['payload/stracker.zip'])) as z:inner={n:z.read(n) for n in z.namelist()}
name='nativePC/plugins/HorizonCollaboration.dll'
inner[name]=(P/'runtime/ninja-horizon-complete-stracker-mapping-fix-20261023/HorizonCollaboration.dll').read_bytes()
inner['StormslingerSupport/Export-Feedback.ps1']=(P/'tools/Export-HorizonStrackerFeedback.ps1').read_bytes()
m=json.loads(inner['StormslingerSupport/release-manifest.json'])
for row in m['files']+m['auxiliary_files']:
 b=inner[row['relative_path']];row.update(size=len(b),sha256=sha(b))
m.update(plugin_sha256=sha(inner[name]),status='mapping_link_fix_live_pending',delivery_revision='unified-mapping-link-fix-20261023')
m['diagnostics']['build']='horizon-complete-stracker-20261023-mapping-link-fix'
inner['StormslingerSupport/release-manifest.json']=(json.dumps(m,indent=2)+'\n').encode()
data=io.BytesIO()
with zipfile.ZipFile(data,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in inner.items():z.writestr(n,b)
outer['payload/stracker.zip']=data.getvalue()
(S/'stracker-fixed.zip').write_bytes(outer['payload/stracker.zip'])
script=(old/'Install-Patch.cmd').read_text(encoding='utf-8');assert before in script
(D/'Install-Patch.cmd').write_text(script.replace(before,sha(outer['payload/stracker.zip'])),encoding='utf-8',newline='\r\n')
helper=P/'tools/Install-HorizonUnifiedPatch.ps1';script=helper.read_text(encoding='utf-8-sig');assert before in script
helper.write_text(script.replace(before,sha(outer['payload/stracker.zip'])),encoding='utf-8-sig',newline='\r\n')
archive=D/(BASE+' - Unified Edition.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_STORED) as z:
 for n,b in outer.items():z.writestr(n,b)
doc=(ROOT/'Unified Edition'/'安装说明 - Installation Instructions.txt').read_text(encoding='utf-8-sig')
doc='任务映射链接修复候选：离线回归通过后仍需实机确认首次提示与功能。 / Mapping-link fix candidate: live confirmation of the first-launch notice and features is still required after offline regression checks.\n\n'+doc
(D/'安装说明 - Installation Instructions.txt').write_text(doc,encoding='utf-8-sig',newline='\r\n')
builder=P/'tools/build_horizon_unified_delivery.py';builder.write_text(builder.read_text().replace('staging/Horizon-startup-diagnostic-20261023/stracker-diagnostic.zip','staging/Horizon-mapping-link-fix-20261023/stracker-fixed.zip'))
source=S/'source-mapping-link-fix.zip'
with zipfile.ZipFile(P/'staging/Horizon-startup-diagnostic-20261023/source-startup-diagnostic.zip') as z:entries={n:z.read(n) for n in z.namelist()}
for n in list(entries):
 if n not in ['source-manifest.json','BUILD.txt'] and (P/n).is_file():entries[n]=(P/n).read_bytes()
for n in ['tools/build-horizon-complete-stracker-mapping-fix-20261023.cmd','tools/package_horizon_mapping_link_fix.py']:
 entries[n]=(P/n).read_bytes()
entries['BUILD.txt']=b'Run tools/build-horizon-complete-stracker-mapping-fix-20261023.cmd. Plugin mapping implementation now matches the tested shared-view implementation. Live acceptance pending.\n'
entries['source-manifest.json']=json.dumps([dict(path=n,sha256=sha(b)) for n,b in sorted(entries.items()) if n not in ['source-manifest.json','BUILD.txt']],indent=2).encode()
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in entries.items():z.writestr(n,b)
report=dict(status='fix_built_install_checks_pending',directory=str(D),archive_sha256=sha(archive.read_bytes()),plugin_sha256=sha(inner[name]),source_sha256=sha(source.read_bytes()),native_payload_unchanged=True,game_modified=False,live_tested=False,offline_regression_reproduced=True)
(S/'fix-index.json').write_text(json.dumps(report,indent=2))
f=P/'staging/Horizon-unified-delivery-20261023/delivery-index.json';index=json.loads(f.read_text());index.update(status=report['status'],directory=str(D),archive_sha256=report['archive_sha256'],installer_sha256=sha((D/'Install-Patch.cmd').read_bytes()),current_fix='staging/Horizon-mapping-link-fix-20261023/fix-index.json',bug_fixed='offline_regression_fixed_live_pending')
index['inputs']['__PLUGIN_ZIP_HASH__']=sha(outer['payload/stracker.zip'])
f.write_text(json.dumps(index,indent=2));print(json.dumps(report))
