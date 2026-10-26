"""Prepare only the 17 approved Aloy head dependencies and audit hunter models."""
from pathlib import Path
import hashlib,json,struct,zlib
from convert_horizon_other_assets import convert_texture,validate_linear
from stage_horizon_other_models import inspect_model,pc_material_parameters,inspect_material_payload,pc_format_witnesses
from audit_stormbolt_effect_material_texture_table import parse as material_parse
from stage_horizon_other_physics import layout

ROOT=Path(__file__).resolve().parents[1]
CASE=ROOT/'research/horizon-complete-c2-live-failures-20261021'
SOURCE=CASE/'aloy-face-hair-approved-source'
BASE=ROOT/'staging/Horizon-complete-c3-work-20261022/payload/nativePC'
PC=ROOT/'MHWI解包/chunk'
OUT=ROOT/'staging/Horizon-complete-hunter-head-repair-20261022'

def main():
    assert not OUT.exists(),'Preserve prior candidate'
    rows=json.loads((SOURCE/'extraction.json').read_text(encoding='utf-8'))['files']
    assert len(rows)==17 and sum(r['size'] for r in rows)==5779198
    descriptors,identity=pc_material_parameters()
    outputs={};physics=[]
    for row in rows:
        name=row['path'];raw=(SOURCE/name).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==row['sha256']
        suffix=Path(name).suffix
        if suffix=='.tex':
            raw=convert_texture(raw);validate_linear(raw)
        elif suffix in ('.ctc','.ccl'):
            parsed=layout(raw,suffix);assert parsed['remaining_bytes']==0
            physics.append(dict(path=name,**parsed))
        else:assert suffix in ('.mod3','.mrl3')
        outputs[name]=raw
    def load(name):
        if name in outputs:return outputs[name]
        for base in (BASE,PC):
            if (base/name).is_file():return (base/name).read_bytes()
        raise FileNotFoundError(name)
    # Include every hunter model in the current cumulative payload, irrespective
    # of whether selected as equipment or layered appearance by the native table.
    names={p.relative_to(BASE).as_posix() for p in (BASE/'pl').rglob('*.mod3')}
    names.update(name for name in outputs if name.endswith('.mod3'))
    models=[];needed=set();unresolved=set()
    for name in sorted(names):
        model=inspect_model(load(name));mrl_name=Path(name).with_suffix('.mrl3').as_posix()
        raw=load(mrl_name);material=material_parse(raw)
        params=inspect_material_payload(raw,material,descriptors)
        table=int(material['material_table_offset'],16)
        hashes=[struct.unpack_from('<I',raw,table+56*i+4)[0] for i in range(material['material_count'])]
        bindings=[]
        for index in sorted({mesh['material_index'] for mesh in model['meshes']}):
            label=model['material_names'][index];key=zlib.crc32(label.encode('ascii'))^0xffffffff
            matches=[i for i,h in enumerate(hashes) if h==key]
            assert len(matches)==1,(name,label,matches)
            bindings.append(dict(name=label,record=matches[0]))
        for texture in material['textures']:
            try:load(texture['path'])
            except FileNotFoundError:unresolved.add(texture['path'])
        needed.update((mesh['format'],mesh['stride']) for mesh in model['meshes'])
        models.append(dict(path=name,mesh_count=len(model['meshes']),bindings=bindings,parameters=params))
    witnesses,examined=pc_format_witnesses(needed)
    report=dict(status='static_dependency_audit_not_live_accepted',models=models,
                unresolved_textures=sorted(unresolved),physics=physics,
                material_identity=identity,format_witnesses=witnesses,pc_models_examined=examined,
                files=[dict(path=n,size=len(b),sha256=hashlib.sha256(b).hexdigest()) for n,b in outputs.items()],
                game_modified=False)
    OUT.mkdir(parents=True)
    (OUT/'resource-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    assert not unresolved,sorted(unresolved)
    for name,data in outputs.items():
        path=OUT/'nativePC'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    print(json.dumps(dict(files=len(outputs),hunter_models=len(models),unresolved_textures=sorted(unresolved))))

if __name__=='__main__':main()
