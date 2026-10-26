"""Regenerate and compile the partial module; never execute its native code."""
from pathlib import Path
import hashlib
import json
import subprocess
import struct
import sys

P = Path(__file__).resolve().parents[1]
MODULE = P/'staging/Horizon-other-module'
GENERATORS = ('build_horizon_other_guildcard_admission.py', 'build_horizon_other_npc_sound_bindings.py', 'build_horizon_other_shield_binding_guards.py', 'audit_horizon_other_shield_reaction_gate.py', 'build_horizon_other_shield_multiplier_owner.py', 'build_horizon_other_shield_aux_owners.py', 'build_horizon_other_shield_lifecycle_owners.py', 'audit_horizon_other_shield_consumer_inventory.py', 'build_horizon_other_shield_shared_core.py', 'build_horizon_other_executable_identity.py', 'audit_horizon_other_equipment.py', 'audit_horizon_other_quest_classification.py',
              'audit_horizon_other_state_b0_semantics.py', 'audit_horizon_other_sos_notification.py',
              'audit_horizon_other_save_mapping.py', 'audit_horizon_other_mapping_consumers.py',
              'audit_horizon_other_mapping_lifecycle.py',
              'audit_horizon_other_rank_labels.py', 'audit_horizon_other_native_rank_readers.py', 'audit_horizon_other_open_writer.py', 'build_horizon_other_quest_open_requirements.py',
              'build_horizon_other_tree_patch_plan.py', 'build_horizon_other_armor_exclusions.py',
              'audit_horizon_other_palico_native_fields.py', 'build_horizon_other_palico_exclusion_plan.py',
              'build_horizon_other_clone_specs.py', 'build_horizon_other_quest_loader_owners.py',
              'build_horizon_other_text_candidate.py', 'build_horizon_other_quest_project_text.py',
              'audit_horizon_other_pcm_sound_bindings.py')

# General native-flow research reports are retained under research/. They do
# not generate runtime inputs and are not prerequisites of every module build.


def digest(path):
    data = path.read_bytes()
    return {'file': path.relative_to(MODULE).as_posix(), 'bytes': len(data),
            'sha256': hashlib.sha256(data).hexdigest()}


def main():
    receipt_path = MODULE/'merge/catalog-build.json'
    try:
        for generator in GENERATORS:
            subprocess.run([sys.executable, str(P/'tools'/generator)], cwd=P, check=True)
        subprocess.run(['cmd.exe', '/d', '/c', str(P/'tools/build-horizon-other-static.cmd')], cwd=P, check=True)
        subprocess.run(['cmd.exe', '/d', '/c', str(P/'tools/build-horizon-other-link-only.cmd')], cwd=P, check=True)
    except subprocess.CalledProcessError as error:
        receipt_path.write_text(json.dumps({'status': 'build failed; previous artifacts may be stale',
                                           'exit_code': error.returncode, 'command': error.cmd}, indent=2), encoding='utf-8')
        raise
    artifacts = sorted(path for path in (MODULE/'src').iterdir() if path.suffix in ('.h', '.cpp', '.inc', '.asm'))
    artifacts += sorted((MODULE/'build').glob('*.obj'))
    artifacts.append(MODULE/'build/horizon_other_catalog.lib')
    artifacts += sorted((MODULE/'build/link-only').glob('*.obj'))
    linked=MODULE/'build/link-only/horizon_other_linkcheck.dll'
    artifacts.append(linked)
    pe=linked.read_bytes()
    nt=struct.unpack_from('<I',pe,60)[0]
    assert pe[nt:nt+4]==b'PE\0\0' and struct.unpack_from('<H',pe,nt+4)[0]==0x8664
    assert struct.unpack_from('<H',pe,nt+22)[0]&0x2000
    assert struct.unpack_from('<I',pe,nt+24+16)[0]==0, 'Link-only DLL must have no entry point'
    assert struct.unpack_from('<2I',pe,nt+24+112)==(0,0), 'Link-only DLL must expose no startup export'
    assert len(list((MODULE/'build').glob('*.obj'))) == 39
    receipt = {'status': 'generated static library and link-only diagnostic DLL; neither executed nor installed',
               'build_command': 'tools/build_horizon_other_module.py',
               'compile_command': 'tools/build-horizon-other-static.cmd',
               'link_command': 'tools/build-horizon-other-link-only.cmd',
               'link_only': {'exit_code':0,'entry_point_rva':0,'exports':0,'loaded':False,
                             'installable':False,'all_library_objects_included':True,
                             'flags':['WHOLEARCHIVE','OPT:NOREF','NOENTRY'],
                             'dependencies':['MinHook 1.3.4 project vendor sources','kernel32.lib','bcrypt.lib',
                                             'libvcruntime.lib','libucrt.lib','default static C/C++ runtime',
                                             'shared status/timer/loop/sync/remote globals and status/timer/growth/loop/sync/remote MASM sources (one copy only)']},
               'generators': list(GENERATORS), 'exit_code': 0,
               'files': [digest(path) for path in artifacts],
               'integration_pending': ['Shared bootstrap and original-image certification',
                                       'Full native quest prerequisites, admission and persistence',
                                       'Native equipment availability, transaction and special mechanics',
                                       'Final module composition in the user-designated merge chat']}
    receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    subprocess.run([sys.executable, str(P/'tools/build_horizon_other_module_manifest.py')], cwd=P, check=True)
    manifest = json.loads((MODULE/'module-manifest.json').read_text(encoding='utf-8'))
    for path in manifest['merge_inputs']:
        assert (MODULE/path).is_file(), path
    texts = json.loads((MODULE/'merge/text-deltas.json').read_text(encoding='utf-8'))
    coverage = json.loads((P/'research/horizon-other-text-language-coverage.json').read_text(encoding='utf-8'))['coverage']
    full_language_tables = sum(not row['missing_candidate_languages'] for row in coverage)
    sound = json.loads((MODULE/'merge/audio-object-deltas.json').read_text(encoding='utf-8'))
    table = json.loads((MODULE/'merge/equipment-table-baselines.json').read_text(encoding='utf-8'))
    materials = json.loads((MODULE/'merge/model-materials.json').read_text(encoding='utf-8'))
    parameters = [p for source in materials['pc_parameter_bindings']
                  for material in source['materials'] for p in material['parameters']]
    pcm_bindings = json.loads((MODULE/'merge/pcm-sound-bindings.json').read_text(encoding='utf-8'))
    lines = ['# 地平线其它内容模块：当前状态', '',
             '制作中，尚非完整可安装补丁。此文件由tools/build_horizon_other_module.py更新；合并说明中的阶段记录为历史。', '',
             f'- 资源候选：{len(manifest["resources"])}文件，{sum(r["size"] for r in manifest["resources"])}字节。',
             f'- 文本：{len(texts["files"])}份稳定键差量，{sum(len(r["changes"]) for r in texts["files"])}处改动；选中{len(coverage)}类表，其中{full_language_tables}类有14语言候选，其余{len(coverage)-full_language_tables}类仍缺部分语言；项目译文与PC原文保留分别标注。',
             '- 六任务各有14语言文本候选：10种PS原文与4种项目译文/PC原生术语；任务机制及语言排版未动态验证，交付文本另查。',
             '- 六任务资源与分类目录已准备；完整开放、承接、结算与保存尚未接通。',
             f'- 共用音库：{len(sound["files"])}份稳定Sound-ID差量，{sum(len(r["changes"]) for r in sound["files"])}对象；音频原生完整接受与触发仍待完成。',
             f'- 音频数据静态核对：{len(pcm_bindings["converted_banks"])+3}音库、{pcm_bindings["converted_media"]+len(pcm_bindings["body068_media"])+pcm_bindings["streamed_media"]}媒体、{pcm_bindings["sound_references"]+2+pcm_bindings["streamed_sound_references"]}声音引用，无缺失；含2个NPC对话流式包，原生解码接受仍未完成。',
             '- NPC列表/容器/事件/音库/流式包已补入候选模块，114条NPC文件路径无缺失；实际加载选择、事件触发与NPC交互仍未完成。',
             '- 织盾声音静态链：玩家默认WWCTL→公共WWCT→WWEV→事件/动作/容器/媒体已对应；具名播放/停止、引用管理及20处依赖已核对。加载就绪、实际时序及原生PCM接受仍待完成。',
             f'- 保留原PC装备表：{len(table["files"])}份整表与PS一致，本模块不覆盖这些表。',
             f'- 材质参数：{len(materials["pc_parameter_bindings"])}份MRL、{len(parameters)}参数已匹配当前PC描述符并核对role/引用区间；保留{sum(p["role"]==2 and p["serialized_value"]==0 for p in parameters)}个原始空纹理绑定。着色器组合、完整依赖与实际显示仍未完成。',
             '- 静态库39对象：任务目录、资源桥、候选存档映射、等级下限、弓武器树编辑、防具排除结果处理、随从配方编辑、统一装备接入接口、原生函数副本准备、共享装备Hook队列、原函数SHA256核验、任务资源加载/分类Hook队列、映射指令事务、前置任务完成事实读取、原生等级读取、六任务开放维护、原生开放查询Hook、EXE身份核验、统一准备入口、启用撤销协调器、借用共用状态的织盾资格/计时适配、两处原生调用桥接与统一事务返回尾部适配、织盾状态生命周期组合、三处受击增长调用桥接、受击/外观循环/同步生命周期组合、织盾音画循环调用桥接、同步封包/完成调用桥接、远端状态应用桥接、原生中间倍率桥接、受击局部判定桥接、统一织盾装备组件会话、装备原生接口绑定、任务与织盾统一启用撤销、共享状态/事务宿主启动接线、NPC服装81声音路径补接、地平线名片指定条目准入及常驻自由任务条件适配、原教程mode2触发调用桥（生产调用与保存全链待完成）。仅编译，未执行。', '',
             '- 全库仅链接检查通过：包含全部对象与MinHook/CNG依赖；build/link-only内无入口、无导出DLL只是诊断产物，不能作为安装包或加载。', '',
             '## 构建', '',
             '在项目根目录以Python运行`tools/build_horizon_other_module.py`重新生成并构建。源码再生成依赖当前认证EXE、已有有效代码导出、PS提取源与Capstone。编译使用x64 VS开发命令行、VS Installer自动发现，或HORIZON_VCVARS64指定工具链。', '',
             '运行`tools/package_horizon_other_sources.py`生成staging/Horizon-other-source-build.zip。解压后执行build.cmd，可仅依赖VS x64 C++工具链编译冻结的生成源码，无需游戏/PS原件、研究文件或Python；源码再生成仍需原项目。该包不含资源、正式加载器或安装器，不是可安装补丁。', '',
             '入口按顺序重新生成源码与静态证据，编译静态库，更新源/产物哈希、资源清单与本状态文件；不运行游戏、模块自测或目标代码。', '',
             '## 合并约束', '',
             '- 共享任务只注册一次。候选存档映射未获安装许可，不能据容量证明擅自启用。',
             '- 弓和风暴枪必须在同一认证原函数副本中组合编辑，再统一重定位与安装唯一Hook。',
             '- GMD按稳定键、共用音库按Sound-ID合并；不能用候选整文件覆盖其他聊天的修改。',
             '- 本聊天不进行动态验证；最终统一合并由用户指定的另一个聊天执行。', '',
             '## 尚未完成的实现', '']
    lines += ['- '+item for item in manifest['remaining_implementation']]
    (MODULE/'当前状态.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    print(json.dumps({'compile_exit': 0, 'link_only_exit':0, 'objects': 39, 'merge_inputs_present': len(manifest['merge_inputs']),
                      'content_complete': False, 'dynamic_tests_executed': False}))


if __name__ == '__main__':
    main()
