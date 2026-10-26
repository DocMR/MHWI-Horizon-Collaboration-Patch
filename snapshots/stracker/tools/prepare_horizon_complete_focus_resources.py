"""Prepare the approved four Focus pendants; no game writes or extraction."""
from pathlib import Path
import hashlib,json,struct,argparse,zlib
from convert_horizon_other_assets import convert_texture,validate_linear
from stage_horizon_other_models import inspect_model,pc_material_parameters,inspect_material_payload,pc_format_witnesses
from audit_stormbolt_effect_material_texture_table import parse as material_parse
from stage_horizon_other_physics import layout
from build_horizon_other_text_candidate import parse
from merge_horizon_other_gmd import merge

P=Path(__file__).resolve().parents[1]
CASE=P/'research/horizon-complete-c2-live-failures-20261021'
SOURCE=CASE/'focus-approved-source'
OUT=P/'staging/Horizon-complete-focus-repair-20261021'
PC=P/'MHWI解包/chunk'
PROJECT_NAMES={
    'ara':['Focus','Focus (النوع B)','Focus (النوع C)','Focus (النوع A)'],
    'pol':['Focus','Focus — typ B','Focus — typ C','Focus — typ A'],
    'ptB':['Focus','Focus tipo B','Focus tipo C','Focus tipo A'],
    'rus':['Focus','Focus — тип B','Focus — тип C','Focus — тип A'],
}
def sha(b):return hashlib.sha256(b).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=OUT)
    output=parser.parse_args().output.resolve()
    assert output.is_relative_to((P/'staging').resolve())
    assert not output.exists(),'Preserve previous candidate'
    receipt=json.loads((SOURCE/'assets-extraction.json').read_text())['files']
    assert len(receipt)==58
    descriptors,identity=pc_material_parameters()
    outputs={};models=[];materials=[];physics=[];needed=set();refs=set()
    for row in receipt:
        name=row['path'];raw=(SOURCE/name).read_bytes();assert sha(raw)==row['sha256']
        if not name.startswith('wp/charm/'):continue
        ext=Path(name).suffix
        if ext=='.tex':
            data=convert_texture(raw);validate_linear(data)
        else:
            data=raw
            if ext=='.mod3':
                parsed=inspect_model(raw);models.append({'path':name,**parsed})
                needed.update((r['format'],r['stride']) for r in parsed['meshes'])
            elif ext=='.mrl3':
                parsed=material_parse(raw)
                params=inspect_material_payload(raw,parsed,descriptors)
                materials.append({'path':name,'parameters':params})
                refs.update(r['path'] for r in parsed['textures'])
            elif ext in ('.ctc','.ccl'):
                parsed=layout(raw,ext)
                assert parsed['remaining_bytes']==0,(name,parsed)
                physics.append({'path':name,**parsed})
            else:raise ValueError('Unreviewed suffix '+name)
        outputs[name]=data
    assert len(models)==4 and len(materials)==4
    mesh_bindings=[]
    for model in models:
        raw=outputs[str(Path(model['path']).with_suffix('.mrl3')).replace('\\','/')]
        material=material_parse(raw);table=int(material['material_table_offset'],16)
        hashes=[struct.unpack_from('<I',raw,table+56*n+4)[0] for n in range(material['material_count'])]
        bindings=[]
        for index in sorted({mesh['material_index'] for mesh in model['meshes']}):
            name=model['material_names'][index]
            matches=[n for n,value in enumerate(hashes) if value==(zlib.crc32(name.encode('ascii'))^0xffffffff)]
            assert len(matches)==1,(model['path'],name,matches)
            bindings.append({'name':name,'material_record':matches[0]})
        mesh_bindings.append({'model':model['path'],'mesh_count':len(model['meshes']),'bindings':bindings})
    unresolved=sorted(r for r in refs if r not in outputs and not (PC/r).is_file())
    assert not unresolved,unresolved
    witnesses,examined=pc_format_witnesses(needed)
    text_changes=[]
    for source in sorted((SOURCE/'common/text/ps4').glob('charm_*.gmd')):
        language=source.stem.removeprefix('charm_')
        relative=f'common/text/steam/charm_{language}.gmd'
        before=(PC/relative).read_bytes();_,ps_values,ps_keys=parse(source.read_bytes());_,pc_values,pc_keys=parse(before)
        delta=[]
        for index in [87,98,99,100]:
            assert ps_keys[index]==pc_keys[index]
            delta.append({'key':pc_keys[index].decode(),'before':pc_values[index].decode(),'after':ps_values[index].decode()})
        outputs[relative]=merge(before,delta)
        text_changes.append({'path':relative,'provenance':'retained PS original','changes':delta})
    for language,names in PROJECT_NAMES.items():
        relative=f'common/text/steam/charm_{language}.gmd'
        before=(PC/relative).read_bytes();_,values,keys=parse(before)
        delta=[{'key':keys[index].decode(),'before':values[index].decode(),'after':name}
               for index,name in zip([87,98,99,100],names)]
        outputs[relative]=merge(before,delta)
        text_changes.append({'path':relative,'provenance':'project labels; Focus proper name retained; not PS original translation','changes':delta})
    for relative,data in outputs.items():
        target=output/'nativePC'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    report={'status':'resource_candidates_prepared_not_installed_or_live_accepted','models':models,'materials':materials,'physics':physics,
            'used_mesh_material_bindings':mesh_bindings,
            'pc_material_identity':identity,'format_witnesses':witnesses,'pc_models_examined':examined,
            'unresolved_texture_paths':unresolved,'text':text_changes,
            'files':[{'path':k,'sha256':sha(v),'size':len(v)} for k,v in outputs.items()],
            'remaining':['Native forge admission/exclusion and unlock query','Actual model/material/physics preview, equip and save acceptance'],
            'original_parameter_and_recipe_tables_unchanged':True,'game_modified':False}
    (output/'resource-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'files':len(outputs),'models':len(models),'texts':len(text_changes),'unresolved_textures':unresolved}))
if __name__=='__main__':main()
