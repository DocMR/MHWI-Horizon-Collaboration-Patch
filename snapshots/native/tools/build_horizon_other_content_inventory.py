"""Build a source-backed initial content checklist, not a completion manifest."""
from pathlib import Path
import json
import re
import struct

P = Path(__file__).resolve().parents[1]
SOURCE = P / 'research/horizon-other-source/common/text/ps4'


def strings(path):
    data = path.read_bytes()
    assert data[:4] == b'GMD\0'
    count, size = struct.unpack_from('<I', data, 24)[0], struct.unpack_from('<I', data, 32)[0]
    assert 0 < size <= len(data) - 40
    values = data[-size:].split(b'\0')
    assert len(values) == count + 1 and not values[-1]
    return [value.decode('utf-8') for value in values[:-1]]


def main():
    groups = []
    for table, pattern in [('bow', r'^Aloy'), ('armor_series', r'^(Aloy|Shield-Weaver|Banuk)'),
                           ('ot_armor', r'^(Felyne Watcher|Frostclaw)'), ('ot_weapon', r'^(Felyne Watcher|Forgefire)'),
                           ('ot_series', r'^(Watcher|Frostclaw|Forgefire)'),
                           ('ou_bow', r'^Aloy')]:
        path = SOURCE / f'{table}_eng.gmd'
        for index, text in enumerate(strings(path)):
            if re.search(pattern, text):
                groups.append({'table': table, 'string_index': index, 'name': text,
                               'source': str(path.relative_to(P)), 'state': 'PS text confirmed; record/resource/runtime linkage pending'})
    quests = []
    for quest_id in (67101, 67102, 67607, 67802, 67803, 67804):
        path = P / f'PS雷暴资源/nativePS4/common/text/quest/q{quest_id}_eng.gmd'
        if path.is_file():
            values = strings(path)
            quests.append({'quest_id': quest_id, 'title': values[0], 'source': str(path.relative_to(P)),
                           'strings': values, 'state': 'PS quest text confirmed; full task configuration validation pending'})
    # Same validated 69-byte gunner record structure as the existing lbg reader;
    # retain raw identity fields only and require every index to fit the text pool.
    bows = []
    for platform, path in [('PS', P / 'research/horizon-other-source/common/equip/bow.wp_dat_g'),
                           ('PC', P / 'MHWI解包/chunk/common/equip/bow.wp_dat_g')]:
        data = path.read_bytes()
        count = struct.unpack_from('<I', data, 6)[0]
        assert len(data) == 10 + 69 * count
        text_path = SOURCE / 'bow_eng.gmd' if platform == 'PS' else P / 'MHWI解包/chunk/common/text/steam/bow_eng.gmd'
        text = strings(text_path)
        for index in range(count):
            offset = 10 + 69 * index
            identity, name_index, description_index = struct.unpack_from('<HHH', data, offset + 59)
            assert name_index < len(text) and description_index < len(text)
            if platform == 'PS' and text[name_index].startswith('Aloy'):
                bows.append({'platform': platform, 'row': index, 'id': identity,
                             'model_id': struct.unpack_from('<h', data, offset + 6)[0],
                             'name_index': name_index, 'name': text[name_index],
                             'file_offset': offset, 'source': str(path.relative_to(P))})
            elif platform == 'PC' and identity in {r['id'] for r in bows if r['platform'] == 'PS'}:
                bows.append({'platform': platform, 'row': index, 'id': identity,
                             'model_id': struct.unpack_from('<h', data, offset + 6)[0],
                             'name_index': name_index, 'name': text[name_index],
                             'file_offset': offset, 'source': str(path.relative_to(P))})
    report = {'status': 'Initial source inventory; implementation incomplete', 'quests': quests,
              'equipment_text_candidates': groups, 'bow_records': bows,
              'limits': ['String indices are not equipment or item IDs.',
                         'Six quest texts do not prove an exhaustive quest list.',
                         'PC identity matching does not establish recipe availability or working assets.',
                         'Layered armor and Palico full-set behavior still require record and consumer validation.']}
    (P / 'research/horizon-other-content-inventory.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['# 地平线其它内容：首轮证据清单', '', '状态：2026-10-15 已批准执行；当前为源数据盘点，尚未完成移植。', '',
             '## 任务文本已确认', '', '| PS任务号 | 英文原名 | 当前判断 |', '| --- | --- | --- |']
    for row in quests:
        lines.append(f'| {row["quest_id"]} | {row["title"]} | 仅文本及编号；配置、奖励、前置和首通仍需核验 |')
    lines += ['', '## 装备文本候选', '', '| 文本表 | 字符串索引（不是装备ID） | 名称 |', '| --- | --- | --- |']
    for row in groups:
        lines.append(f'| {row["table"]} | {row["string_index"]} | {row["name"]} |')
    lines += ['', '## 弓记录逐项核对', '', '| 平台 | 行 | 装备ID | 模型号 | 名称 |', '| --- | --- | --- | --- | --- |']
    for row in bows:
        lines.append(f'| {row["platform"]} | {row["row"]} | {row["id"]} | {row["model_id"]} | {row["name"]} |')
    lines += ['', '## 两补丁内容归属', '',
              '- 本聊天负责上述其它装备、全部联动任务、素材奖励与原生工坊获取流程，含织盾装备专属机制。',
              '- 完整风暴枪本体及射击、蓄电、自伤、过载等由风暴枪聊天负责。',
              '- 共用任务与配方采用同一份定义，两个安装包的合并与卸载归属待阶段2落实；暂不改写另一聊天运行代码。',
              '', '## 下一步', '',
              '解析弓、防具和随从的配方及原始记录，核对素材ID与模型路径；追踪织盾原生机制；补齐外观、交货委托及全部任务关联。',
              '既有 body068/CombineArmor 证据暂作为候选，须确认装备绑定后才能归入织盾实现；不把名称或类型存在视为机制可用。',
              '', '机器证据：horizon-other-content-inventory.json；来源哈希及提取偏移：horizon-other-source-receipt.json。']
    markdown = P / 'research/地平线其它内容清单.md'
    previous = markdown.read_text(encoding='utf-8') if markdown.exists() else ''
    marker = previous.find('\n## 2026-10-15 配方')
    history = previous[marker:] if marker >= 0 else ''
    markdown.write_text('\n'.join(lines) + '\n' + history, encoding='utf-8')
    print(json.dumps({'quest_texts': len(quests), 'equipment_candidates': len(groups), 'bow_records': bows}, ensure_ascii=False))


if __name__ == '__main__':
    main()
