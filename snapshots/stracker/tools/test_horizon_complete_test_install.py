"""Exercise developer test installation against an isolated game-shaped folder."""
from pathlib import Path
import json,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from horizon_complete_test_install import install_test,restore_test
from patch_uninstall_plan import digest
P=Path(__file__).resolve().parents[1]
def put(root,name,data):
 p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
def main():
 fixture=P/'research/horizon-complete-test-installer-fixture-20261021'
 if fixture.exists():raise ValueError('Preserve existing fixture evidence')
 game=fixture/'game';game.mkdir(parents=True)
 remote=fixture/'remote';remote.mkdir()
 initial={'dinput8.dll':b'prior-loader','amd_ags_x64.dll':b'working-original-amd',
  'nativePC/external/preserved.txt':b'external mod',
  'Export-Feedback.cmd':b'prior feedback entry',
  'StormslingerSupport/release-manifest.json':b'prior support manifest',
  'StormslingerSupport/user-notes.txt':b'keep user note','savedata_backup/save.bin':b'prior backup save'}
 for name,data in initial.items():put(game,name,data)
 put(remote,'SAVEDATA1000',b'before test')
 package=(P/'staging/Horizon-complete-task-test-c1/package').resolve();session=(fixture/'session').resolve()
 state=install_test(package,game.resolve(),session,[remote.resolve(),(game/'savedata_backup').resolve()])
 assert state['status']=='installed_waiting_for_user_start'
 m=json.loads((package/'release-manifest.json').read_text(encoding='utf-8-sig'))
 for row in m['files']:assert digest(game/row['relative_path'])==row['sha256']
 for row in state['support']:assert digest(game/row['relative_path'])==row['installed_sha256']
 assert (game/'amd_ags_x64.dll').read_bytes()==initial['amd_ags_x64.dll']
 # Real test progress must survive restoration; no rollback from save-before.
 put(remote,'SAVEDATA1000',b'latest post-test progress')
 put(game,'savedata_backup/save.bin',b'latest in-game backup')
 changed='nativePC/quest/questData_67101.mib';original=(game/changed).read_bytes();put(game,changed,b'user changed this file')
 try:restore_test(session)
 except ValueError as error:assert 'Changed after test installation' in str(error)
 else:raise AssertionError('Changed payload was overwritten')
 assert (game/changed).read_bytes()==b'user changed this file'
 assert not (session/'save-after.json').exists()
 put(game,changed,original)
 restored=restore_test(session);assert restored['status']=='restored'
 assert (remote/'SAVEDATA1000').read_bytes()==b'latest post-test progress'
 assert (game/'savedata_backup/save.bin').read_bytes()==b'latest in-game backup'
 expected=dict(initial);expected['savedata_backup/save.bin']=b'latest in-game backup'
 actual={p.relative_to(game).as_posix():p.read_bytes() for p in game.rglob('*') if p.is_file()}
 assert actual==expected,sorted(set(actual)^set(expected))
 result={'status':'fixture_install_restore_passed','payload_files':len(m['files']),'support_files':len(state['support']),
 'covered':['515 candidate payload hashes','6 support files and candidate feedback/uninstall manifest installed together',
 'Changed owned file blocks restoration before any restoration writes',
 'Overwritten loader/support restored and added files removed','Working AMD and external MOD unchanged',
 'Latest Steam-shaped and savedata_backup-shaped saves preserved; never restored save-before'],
 'actual_game_or_saves_modified':False,'developer_tool_only_not_release_installer':True}
 (P/'research/horizon-complete-test-installer-tests-20261021.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print(json.dumps(result))
if __name__=='__main__':main()
