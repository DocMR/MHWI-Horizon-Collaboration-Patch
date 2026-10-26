"""Static Shield-Weaver serialized association and bounded query-site audit."""
import hashlib,json,re,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
from audit_horizon_other_equipment import rows,ARMOR_FMT
from build_horizon_other_text_candidate import parse

def main():
    im=CodeImage(); roots=[P/'research/horizon-other-source',P/'MHWI解包/chunk']
    armor=[]; tables=[]
    for root in roots:
        data,records=rows(root/'common/equip/armor.am_dat',ARMOR_FMT)
        selected=[(n,r) for n,r in enumerate(records) if r[-4]==523]
        assert len(selected)==5 and sorted(r[5] for _,r in selected)==list(range(5))
        assert all(tuple(r[7:9])==(68,10) and tuple(r[22:24])==(209,1) for _,r in selected)
        armor.append({'root':str(root.relative_to(P)),'rows':[n for n,_ in selected],
                      'raw':[struct.pack(ARMOR_FMT,*r).hex() for _,r in selected]})
    assert armor[0]['raw']==armor[1]['raw']
    for relative,size in [('common/equip/skill_data.skl_dat',35),('common/equip/skill_point_data.skl_pt_dat',2)]:
        payloads=[(root/relative).read_bytes() for root in roots]
        assert payloads[0]==payloads[1]
        data=payloads[0];count=struct.unpack_from('<I',data,6)[0]
        assert len(data)==10+size*count
        selected=[{'index':n,'raw':data[10+n*size:10+(n+1)*size].hex()} for n in range(count)
                  if (struct.unpack_from('<H',data,10+n*size)[0]==209 if size==35 else n==209)]
        tables.append({'path':relative,'count':count,'sha256':hashlib.sha256(data).hexdigest(),
                       'ps_pc_identical':True,'selected':selected})
    assert [r['index'] for r in tables[0]['selected']]==[467]
    labels=[]
    for table,indices in [('skill',[934,935]),('skill_pt',[627,628,629])]:
        _,values,keys=parse((roots[0]/f'common/text/vfont/{table}_eng.gmd').read_bytes())
        labels += [{'table':table,'index':i,'key':keys[i].decode(),'text':values[i].decode()} for i in indices]
    getter=im.instructions(0x12F80F0)
    assert len(getter)==4 and getter[2].op_str=='eax, dword ptr [rcx + rdx*8 + 0x84]'
    candidates=[]; undecoded=[]
    for m in re.finditer(b'\xba\xd1\x00\x00\x00',im.code):
        r=im.base+m.start();f=im.owner(r)
        if not f: continue
        try: _,body=decode_group(im,f[0])
        except (ValueError,AssertionError): undecoded.append(hex(r));continue
        ix=next((n for n,i in enumerate(body) if i.address==r and i.mnemonic=='mov' and i.op_str=='edx, 0xd1'),None)
        if ix is None:continue
        window=body[ix:ix+9]
        calls=[i for i in window[1:] if i.mnemonic in ('call','jmp')]
        candidates.append({'site':hex(r),'owner':hex(f[0]),
                           'first_transfer':calls[0].op_str if calls else None,
                           'window':[im.format(i) for i in window]})
    direct=[r for r in candidates if r['first_transfer'] in ('0x12f80f0','0x1922190')]
    report={'status':'serialized_binding_verified_native_effect_incomplete','exe_sha256':im.meta['exe_sha256'],
        'armor':armor,'tables':tables,'source_labels':labels,
        'native_selected_record_getter':[im.format(i) for i in getter],
        'candidate_runtime_selected_record_offset':hex(0x84+24*209),
        'immediate_edx_209_scan':{'certified_sites':candidates,'undecoded_sites':undecoded,
                                 'direct_known_query_candidates':direct},
        'limits':['Serialized armor field naming follows existing schema; native equipment-to-skill accumulation not certified.',
                  'Record467 text934 and point209 text627 associations are serialized/index candidates, not native text lookup proof.',
                  'Raw record second word5 is retained without assigning activation-threshold semantics.',
                  'EDX immediate scan is not exhaustive: other registers, propagated constants, direct slots and dynamic calls remain.',
                  'No matching immediate query does not prove PC effect absent; CombineArmor identity remains unproven.',
                  'Damage reduction, reaction suppression, hit depletion, recovery and lifecycle require native PS/PC comparison.',
                  'No target code executed; no game files written.']}
    (P/'research/horizon-other-shield-skill-binding.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'armor_pieces':5,'skill_record':467,'point_id':209,'tables_identical':True,
                      'immediate_sites':len(candidates),'undecoded_sites':len(undecoded),
                      'direct_known_query_candidates':len(direct),'effect_complete':False}))
if __name__=='__main__':main()
