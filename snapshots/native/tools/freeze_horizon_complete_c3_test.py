"""Freeze C2 feedback repairs for developer-managed acceptance; never install."""
from pathlib import Path
import json,hashlib,shutil,zipfile
from stormslinger_release_manager import verify
from horizon_complete_test_install import support_files

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'staging/Horizon-complete-cumulative-c2-r2-20261021/package'
WORK=ROOT/'staging/Horizon-complete-c3-work-r2-20261022/payload'
OUT=ROOT/'staging/Horizon-complete-cumulative-c3-20261022'
DLL=ROOT/'runtime/ninja-horizon-complete-c3-repairs-20261021/dinput8.dll'
LABEL='C3'
SOURCE_TAG='c3'
EXTRA_MANIFEST={}
BUILD_SCRIPT=ROOT/'tools/build-horizon-complete-repairs-runtime-20261021.cmd'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    assert not OUT.exists(),'Preserve frozen candidate'
    old,_,_=verify(BASE)
    package=OUT/'package'
    shutil.copytree(WORK,package/'payload')
    shutil.copy2(DLL,package/'payload/dinput8.dll')
    shutil.copytree(BASE/'support-root',package/'support-root')
    manifest=dict(old)
    manifest.update(status='private_c3_feedback_repair_candidate_pending_live_acceptance',
        candidate_resources='c7_plus_focus_tutorial110_aloy_head_dependencies',
        runtime_build='horizon-complete-20261022-c3-repairs',
        standalone_dll_sha256=sha(DLL),full_project_complete=False,
        live_acceptance='C2 failed; C3 repairs not yet installed or accepted',
        first_entry_notice='Six pages; native panel layout and same-language button repaired; new C3 acknowledgement version',
        remaining_project_items=['Concentrated live acceptance of all six hunter armor/layered groups, four Focus pendants and tutorial',
          'Six-page notice layout/language/resolution acceptance',
          'Recheck equipment-triggered Esc/departure hang and resolve remaining findings',
          'Remaining full-content acceptance and final cross-PC delivery'],
        files=[dict(relative_path=p.relative_to(package/'payload').as_posix(),size=p.stat().st_size,sha256=sha(p))
               for p in sorted((package/'payload').rglob('*')) if p.is_file()])
    manifest['diagnostics']=dict(manifest.get('diagnostics',{}),build=manifest['runtime_build'])
    manifest.update(EXTRA_MANIFEST)
    manifest['diagnostics']=dict(manifest.get('diagnostics',{}),build=manifest['runtime_build'])
    raw=(json.dumps(manifest,indent=2)+'\n').encode('utf-8')
    (package/'release-manifest.json').write_bytes(raw)
    (package/'support-root/StormslingerSupport/release-manifest.json').write_bytes(raw)
    verify(package);support_files(package,manifest)
    sources=[]
    for folder in ['runtime/src','runtime/vendor','staging/Horizon-complete-integration-20261021/src','staging/Horizon-other-module/src']:
        sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts and '.git' not in p.parts)
    sources.extend((ROOT/'tools').glob('*.cpp'))
    sources.extend((ROOT/'tools').glob('*horizon*.*'))
    sources.extend([ROOT/'runtime/CMakeLists.txt',BUILD_SCRIPT])
    sources=sorted(set(sources))
    archive=OUT/('source-'+SOURCE_TAG+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for path in sources:z.write(path,path.relative_to(ROOT).as_posix())
        z.writestr('BUILD.txt',LABEL+' acceptance candidate; not a final release.\nDLL SHA256 '+sha(DLL)+'\nRun '+BUILD_SCRIPT.relative_to(ROOT).as_posix()+' from the project root in the supported x64 MSVC environment. No player saves or logs included.\n')
        z.writestr('source-manifest.json',json.dumps([dict(path=p.relative_to(ROOT).as_posix(),sha256=sha(p)) for p in sources],indent=2))
    summary=dict(status='frozen_not_installed_not_live_accepted',payload_files=len(manifest['files']),
                 support_files=len(support_files(package,manifest)),dll_sha256=sha(DLL),
                 source_sha256=sha(archive),manifest_sha256=sha(package/'release-manifest.json'),
                 game_modified=False,saves_modified=False)
    (OUT/'candidate-index.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
    print(json.dumps(summary))

if __name__=='__main__':main()
