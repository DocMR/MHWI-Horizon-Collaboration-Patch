from pathlib import Path
import hashlib,json,zipfile
P=Path(__file__).resolve().parents[1]
S=P/'staging/Horizon-complete-stracker-stage2-20261023'
R=P/'research/horizon-stracker-stage2-20261023'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
tests=json.loads((P/'research/horizon-stracker-stage2-delivery-tests-20261023-r3/results.json').read_text())
assert tests['status']=='passed'
source=S/'source-stracker-stage2.zip'
with zipfile.ZipFile(source) as z:entries={n:z.read(n) for n in z.namelist()}
for name in ['tools/test_horizon_stracker_delivery.py','tools/finalize_horizon_stracker_stage2.py']:
    entries[name]=(P/name).read_bytes()
entries['source-manifest.json']=json.dumps([dict(path=n,sha256=hashlib.sha256(b).hexdigest()) for n,b in sorted(entries.items()) if n not in ['source-manifest.json','BUILD.txt']],indent=2).encode()
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
    for n,b in sorted(entries.items()):z.writestr(n,b)
index=json.loads((S/'candidate-index.json').read_text())
index.update(status='offline_passed_live_pending',source_sha256=sha(source),delivery_tests='research/horizon-stracker-stage2-delivery-tests-20261023-r3/results.json',selftests=['HorizonCompleteQuestLoaderMappingSelftest','QuestLoaderMappingCompatibilitySelftest','HorizonCompleteShieldCallbacksSelftest','HorizonQuestLoaderOwnerCopySelftest'],live_tested=False)
(S/'candidate-index.json').write_text(json.dumps(index,indent=2))
(R/'results.json').write_text(json.dumps(index,indent=2))
note='''

### 2026-10-23 保留原DLL版第2大项完成：离线通过，待批准实机
完整插件已补接QuestLoader共享任务表，六联动任务与外部任务保留；完整模块读取同一表和引用状态，不另发原生映射。已识别QuestLoader修改的入口只在私有认证副本中还原，原插件不改写，完整owner SHA继续核对。完整映射、旧兼容映射、织盾者回调、异常跳转拒绝四项测试通过。
独立候选目录：Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch - Stracker Edition。ZIP不含根dinput8.dll、loader.dll、配置或AMD DLL；安装器核验受支持原加载器与QuestLoader版本，未知版本拒绝且不写文件。隔离目录真实安装/取消、反馈导出、卸载与恢复原覆盖文件通过；原加载器、QuestLoader、其它文件及存档保持原样。夹具使用假EXE并模拟UI/进程，不代表实机通过。证据research/horizon-stracker-stage2-delivery-tests-20261023-r3/results.json；初两轮失败为夹具参数/自删除模拟遗漏，记录保留。
候选、配对源码、哈希与证据封存在staging/Horizon-complete-stracker-stage2-20261023/candidate-index.json。原生公开包未更改，游戏目录未改。第三项统一实机验收须用户批准：确认原加载器/插件、标题提示/Esc、六任务及保存、雷暴连射/织盾者/换装、反馈、安装卸载再安装；开始前一次性发送操作和预期。尚不能公开为已验收兼容成品，不承诺任意外部MOD兼容。
'''
for name in ['项目进度.md','项目目标.md','新聊天交接.md','保留原DLL版-安装包设计方案.md']:
    with (P/name).open('a',encoding='utf-8') as f:f.write(note)
print(json.dumps(index))
