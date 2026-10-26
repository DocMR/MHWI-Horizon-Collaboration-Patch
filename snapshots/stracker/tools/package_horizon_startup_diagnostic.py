"""Replace only the plugin payload with a diagnostic build; no game install."""
from pathlib import Path
import io,hashlib,json,zipfile,shutil
P=Path(__file__).resolve().parents[1]
BASE='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
D=P/(BASE+' - Release Packages')/'Unified Edition'
S=P/'staging/Horizon-startup-diagnostic-20261023'
S.mkdir(exist_ok=True)
def digest(b):return hashlib.sha256(b).hexdigest()
archive=D/(BASE+' - Unified Edition.zip')
with zipfile.ZipFile(archive) as z:outer={n:z.read(n) for n in z.namelist()}
old_plugin_hash=digest(outer['payload/stracker.zip'])
original=D
D=D.parent/'Startup Diagnostic'
D.mkdir(exist_ok=True)
for file in ['Install-Patch.cmd','安装说明 - Installation Instructions.txt']:shutil.copy2(original/file,D/file)
archive=D/(BASE+' - Unified Edition.zip')
with zipfile.ZipFile(io.BytesIO(outer['payload/stracker.zip'])) as z:inner={n:z.read(n) for n in z.namelist()}
name='nativePC/plugins/HorizonCollaboration.dll'
inner[name]=(P/'runtime/ninja-horizon-complete-stracker-startup-diagnostic-20261023/HorizonCollaboration.dll').read_bytes()
m=json.loads(inner['StormslingerSupport/release-manifest.json'])
for row in m['files']:
 if row['relative_path']==name:row.update(size=len(inner[name]),sha256=digest(inner[name]))
m.update(plugin_sha256=digest(inner[name]),status='startup_failure_diagnostic_pending',delivery_revision='unified-startup-diagnostic-20261023')
m['diagnostics']['build']='horizon-complete-stracker-20261023-startup-diagnostic'
inner['StormslingerSupport/release-manifest.json']=(json.dumps(m,indent=2)+'\n').encode()
data=io.BytesIO()
with zipfile.ZipFile(data,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in inner.items():z.writestr(n,b)
outer['payload/stracker.zip']=data.getvalue()
(S/'stracker-diagnostic.zip').write_bytes(outer['payload/stracker.zip'])
for path,encoding in [(P/'tools/Install-HorizonUnifiedPatch.ps1','utf-8-sig'),(D/'Install-Patch.cmd','utf-8')]:
 text=path.read_text(encoding=encoding);assert old_plugin_hash in text
 path.write_text(text.replace(old_plugin_hash,digest(outer['payload/stracker.zip'])),encoding=encoding,newline='\r\n')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_STORED) as z:
 for n,b in outer.items():z.writestr(n,b)
# Ensure future unified rebuilds retain this plugin payload and updated support text.
builder=P/'tools/build_horizon_unified_delivery.py'
text=builder.read_text();old="plugin=OLD/(BASE+' - Compatible Edition')/(BASE+' - Stracker Edition.zip')"
assert old in text
builder.write_text(text.replace(old,"plugin=P/'staging/Horizon-startup-diagnostic-20261023/stracker-diagnostic.zip'"))
doc=D/'安装说明 - Installation Instructions.txt';text=doc.read_text(encoding='utf-8-sig')
text='启动诊断候选：正在定位Stracker方式首次不显示提示的问题，尚未确认修复。 / Startup diagnostic candidate: the missing first-launch notice in Stracker mode is under investigation and is not yet confirmed fixed.\n\n'+text
doc.write_text(text,encoding='utf-8-sig',newline='\r\n')
source=S/'source-startup-diagnostic.zip'
with zipfile.ZipFile(P/'staging/Horizon-complete-stracker-stage2-20261023/source-stracker-stage2.zip') as z:entries={n:z.read(n) for n in z.namelist()}
for n in list(entries):
 if n not in ['source-manifest.json','BUILD.txt'] and (P/n).is_file():entries[n]=(P/n).read_bytes()
for n in ['tools/build-horizon-complete-stracker-startup-diagnostic-20261023.cmd','tools/package_horizon_startup_diagnostic.py']:
 entries[n]=(P/n).read_bytes()
entries['source-manifest.json']=json.dumps([dict(path=n,sha256=digest(b)) for n,b in sorted(entries.items()) if n not in ['source-manifest.json','BUILD.txt']],indent=2).encode()
entries['BUILD.txt']=b'Run tools/build-horizon-complete-stracker-startup-diagnostic-20261023.cmd. Startup failure investigation; not a verified fix.\n'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in entries.items():z.writestr(n,b)
report=dict(status='diagnostic_built_verification_pending',archive_sha256=digest(archive.read_bytes()),plugin_sha256=digest(inner[name]),source_sha256=digest(source.read_bytes()),native_payload_unchanged=True,game_modified=False,bug_fixed=False,requires_user_startup=True)
(S/'diagnostic-index.json').write_text(json.dumps(report,indent=2))
index_path=P/'staging/Horizon-unified-delivery-20261023/delivery-index.json'
index=json.loads(index_path.read_text());index.update(directory=str(D),status=report['status'],archive_sha256=report['archive_sha256'],installer_sha256=digest((D/'Install-Patch.cmd').read_bytes()),current_diagnostic='staging/Horizon-startup-diagnostic-20261023/diagnostic-index.json',bug_fixed=False)
index['inputs']['__PLUGIN_ZIP_HASH__']=digest(outer['payload/stracker.zip'])
index_path.write_text(json.dumps(index,indent=2))
print(json.dumps(report))
