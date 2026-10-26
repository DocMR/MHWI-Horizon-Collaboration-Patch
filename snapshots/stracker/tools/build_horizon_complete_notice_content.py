"""Compose the user-selected six pages from existing localized candidate GMDs.

Data preparation only: no game writes, window creation or trigger installation.
"""
from pathlib import Path
import hashlib
import json
from build_horizon_other_text_candidate import parse

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'staging/Horizon-complete-tutorial-payload-20261021-c7/nativePC'
LANGUAGES = ('ara','chS','chT','eng','fre','ger','ita','jpn','kor','pol','ptB','rus','spL','spa')
HEADINGS = {
 'ara':['المهام المضافة','الأسلحة المضافة','الدروع المضافة','المظاهر المضافة','معدات الباليكو المضافة','الوضعيات المضافة','الألقاب المضافة'],
 'chS':['追加任务','追加武器','追加防具','追加外观','追加随从装备','追加姿势','追加称号'],
 'chT':['追加任務','追加武器','追加防具','追加外觀','追加隨從裝備','追加姿勢','追加稱號'],
 'eng':['Added Quests','Added Weapons','Added Armor','Added Layered Equipment','Added Palico Equipment','Added Poses','Added Titles'],
 'fre':['Quêtes ajoutées','Armes ajoutées','Armures ajoutées','Équipements spéciaux ajoutés','Équipement Palico ajouté','Poses ajoutées','Titres ajoutés'],
 'ger':['Zusätzliche Quests','Zusätzliche Waffen','Zusätzliche Rüstungen','Zusätzliche Dekorausrüstung','Zusätzliche Palico-Ausrüstung','Zusätzliche Posen','Zusätzliche Titel'],
 'ita':['Missioni aggiunte','Armi aggiunte','Armature aggiunte','Equipaggiamento estetico aggiunto','Equipaggiamento compagno aggiunto','Pose aggiunte','Titoli aggiunti'],
 'jpn':['追加クエスト','追加武器','追加防具','追加重ね着装備','追加オトモ装備','追加ポーズ','追加称号'],
 'kor':['추가 퀘스트','추가 무기','추가 방어구','추가 덧입는 장비','추가 동반자 장비','추가 포즈','추가 칭호'],
 'pol':['Dodane zadania','Dodana broń','Dodane pancerze','Dodane wyposażenie warstwowe','Dodane wyposażenie koleżkota','Dodane pozy','Dodane tytuły'],
 'ptB':['Missões adicionadas','Armas adicionadas','Armaduras adicionadas','Equipamentos em camadas adicionados','Equipamentos de Amigato adicionados','Poses adicionadas','Títulos adicionados'],
 'rus':['Добавленные задания','Добавленное оружие','Добавленные доспехи','Добавленное многослойное снаряжение','Добавленное снаряжение палико','Добавленные позы','Добавленные титулы'],
 'spL':['Misiones añadidas','Armas añadidas','Armaduras añadidas','Equipo superpuesto añadido','Equipo de camarada añadido','Poses añadidas','Títulos añadidos'],
 'spa':['Misiones añadidas','Armas añadidas','Armaduras añadidas','Equipo superpuesto añadido','Equipo de camarada añadido','Poses añadidas','Títulos añadidos'],
}
CATEGORIES = ('quests','weapons','armor','layered','palico','pose','titles')
PARTS = ('HEAD','BODY','ARM','WAIST','LEG')

def equipment(table, keys):
    return [(f'common/text/steam/{table}_{{language}}.gmd', key) for key in keys]

def armor(ids):
    return equipment('armor', [f'AM_{part}{227 if idx==225 and part=="WAIST" else idx}_NAME'
                               for idx in ids for part in PARTS])

PAGES = [
    [('quests', [(f'common/text/quest/q{i}_{{language}}.gmd', None)
                 for i in (67101,67102,67607,67802,67803,67804)]),
     ('weapons', equipment('bow',[f'WP_BOW_{i}_NAME' for i in (102,103,254)])+
      equipment('lbg',['WP_LBG_238_NAME','WP_LBG_239_NAME']))],
    [('armor',armor((225,298)))],
    [('armor',armor((523,532)))],
    [('layered',armor((299,718))+equipment('ou_bow',['WP_BOW_OUTER_066_NAME']))],
    [('palico',sum((equipment('ot_weapon',[f'OWEP{i}_NAME'])+
       equipment('ot_armor',[f'OAM_HEAD{i}_NAME',f'OAM_BODY{i}_NAME'])
       for i in ('059','166','189')),[]))],
    [('pose',equipment('GC_Pose',['GC_Pose_13_id'])),
     ('titles',equipment('GC_Title_1',[f'GC_Title_1_{i}_id' for i in
                         (782,783,784,785,786,1500,1501,1502,1503,1504)]))],
]

def main():
    cache={};sources={};localized={}
    for language in LANGUAGES:
        pages=[]
        for sections in PAGES:
            blocks=[]
            for category,entries in sections:
                values=[]
                for pattern,key in entries:
                    relative=pattern.format(language=language)
                    if relative not in cache:
                        raw=(BASE/relative).read_bytes()
                        _,pool,mapping=parse(raw)
                        cache[relative]=({k.decode():pool[i].decode('utf-8') for i,k in mapping.items()},
                                         pool[0].decode('utf-8'))
                        sources[relative]=hashlib.sha256(raw).hexdigest()
                    table,first=cache[relative]
                    value=table[key] if key else first
                    assert value.strip() and 'HARDUMMY' not in value and '\0' not in value,(relative,key)
                    values.append(value)
                blocks.append({'category':category,'entries':values})
            pages.append(blocks)
        assert [sum(len(b['entries']) for b in page) for page in pages]==[11,10,10,11,9,11]
        localized[language]=pages
    # Keep the authored categories separate from original proper names. These
    # identifiers are not player-facing translated headings or UI strings.
    result={'status':'localized_names_prepared_not_presented',
            'trigger_requirement':'before_title_screen',
            'pagination':'user-selected6 pages; tasks and weapons together',
            'headings':'category identifiers only; presentation binding pending',
            'languages':localized,'sources':sources}
    path=ROOT/'staging/Horizon-complete-delivery-20261021/notice-content-six-pages.json'
    path.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    # Embed source-derived names as literal UTF-8 bytes: no runtime JSON/Python
    # dependency and no compiler source-code-page dependency on another PC.
    lines=['// Generated from retained localized GMDs by build_horizon_complete_notice_content.py.',
           'inline constexpr std::array<NoticeLocalizedPages,14> notice_localized_pages{{']
    for language,pages in localized.items():
        lines.append('{"'+language+'",{{')
        for page in pages:
            parts=[]
            for block in page:
                heading=HEADINGS[language][CATEGORIES.index(block['category'])]
                parts.append(heading+'\n'+'\n'.join('· '+s for s in block['entries']))
            text='\n\n'.join(parts)
            lines.append('"'+''.join(f'\\x{b:02x}' for b in text.encode('utf-8'))+'",')
        lines.append('}}},')
    lines.append('}};')
    lines.append('inline constexpr std::array<NoticeLocalizedClose,14> notice_localized_close{{')
    for language in LANGUAGES:
        relative=f'common/text/common_{language}.gmd'
        raw=(ROOT/'MHWI解包/chunk'/relative).read_bytes()
        _,pool,mapping=parse(raw)
        names={key.decode():pool[index].decode('utf-8') for index,key in mapping.items()}
        label=names['CLOSE']
        assert label and '\0' not in label
        lines.append('{"'+language+'","'+''.join(f'\\x{b:02x}' for b in label.encode('utf-8'))+'"},')
    lines.append('}};')
    (ROOT/'staging/Horizon-complete-integration-20261021/src/generated_notice_page_text.inc').write_text('\n'.join(lines)+'\n',encoding='ascii')
    print(f'Prepared {len(localized)} languages x6 pages x62 entries from {len(sources)} GMD files.')

if __name__=='__main__':main()
