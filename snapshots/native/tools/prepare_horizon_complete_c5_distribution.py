"""Prepare a private C5 archive from the frozen package; no installation/publication."""
from pathlib import Path
import argparse,hashlib,json,shutil,zipfile
from stormslinger_release_manager import verify
from horizon_complete_test_install import support_files
P=Path(__file__).resolve().parents[1]
NAME='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--candidate',choices=['c5','c6','c8'],default='c5');args=parser.parse_args();label=args.candidate.upper()
 package=P/f'staging/Horizon-complete-cumulative-{args.candidate}-20261022/package';manifest,_,_=verify(package);support=support_files(package,manifest)
 output=package.parent/'distribution'/NAME;assert not output.exists();(output/'StormslingerInstaller').mkdir(parents=True)
 archive=output/(NAME+'.zip')
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
  for root in [package/'payload',package/'support-root']:
   for path in sorted(root.rglob('*')):
    if path.is_file():z.write(path,path.relative_to(root).as_posix())
 expected={r['relative_path']:r['sha256'] for r in manifest['files']+support}
 with zipfile.ZipFile(archive) as z:
  assert set(z.namelist())==set(expected)
  for name,digest in expected.items():assert hashlib.sha256(z.read(name)).hexdigest()==digest
  assert 'dinput8.dll' in z.namelist() and any(n.startswith('nativePC/') for n in z.namelist())
  assert not any(n.endswith('.txt') and 'LICENSE' not in n for n in z.namelist())
 shutil.copy2(P/'tools/Install-HorizonPatch.cmd',output/'Install-Patch.cmd')
 shutil.copy2(P/'tools/Install-HorizonPatch.ps1',output/'StormslingerInstaller/Install-Patch.ps1')
 instruction=P/'staging/Horizon-complete-cumulative-c2-r2-20261021/distribution'/NAME/'安装说明 - Installation Instructions.txt'
 text=instruction.read_text(encoding='utf-8-sig');text=text.replace('内部累计测试候选，尚非公开成品。',f'{label}内部累计测试候选，尚非公开成品。').replace('Private cumulative test candidate, not a public release.',f'Private {label} cumulative test candidate, not a public release.')
 (output/'安装说明 - Installation Instructions.txt').write_text(text,encoding='utf-8-sig',newline='\r\n')
 report={'status':f'private_{args.candidate}_distribution_prepared_pending_live_acceptance','archive_files':len(expected),'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'zip_root_direct_nativePC_dll':True,'instruction_outside_zip':True,'installer_sources':['tools/Install-HorizonPatch.cmd','tools/Install-HorizonPatch.ps1'],'game_modified':False,'old_public_delivery_unchanged':True}
 (package.parent/'distribution-preview-index.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report))
if __name__=='__main__':main()
