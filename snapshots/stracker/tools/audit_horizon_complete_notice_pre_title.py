"""Trace retained startup/title state bodies without decoding alignment padding."""
from pathlib import Path
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P

def trace(im,start,switch,count=7,extra_switches=()):
    offset=switch-im.base
    assert 0 not in im.valid[offset:offset+count*4]
    targets=struct.unpack_from(f'<{count}I',im.code,offset)
    assert all(start<=a<switch for a in targets)
    pending=[start,*targets];nested=[]
    for site,table,n in extra_switches:
        at=table-im.base
        assert 0 not in im.valid[at:at+n*4]
        more=struct.unpack_from(f'<{n}I',im.code,at)
        assert all(start<=a<switch for a in more)
        pending.extend(more)
        nested.append({'site':hex(site),'table':hex(table),'states':[hex(a) for a in more]})
    decoded={};occupied={};failures=[];exits=[]
    while pending:
        pc=pending.pop()
        while start<=pc<switch and pc not in decoded:
            o=pc-im.base
            ins=next(im.md.disasm(im.code[o:min(o+15,switch-im.base)],pc,count=1),None)
            if not ins or 0 in im.valid[o:o+ins.size]:
                failures.append(hex(pc));break
            assert all(n not in occupied or occupied[n]==pc for n in range(pc,pc+ins.size))
            decoded[pc]=ins
            for n in range(pc,pc+ins.size):occupied[n]=pc
            if ins.mnemonic.startswith('j'):
                if ins.operands[0].type==2:
                    target=ins.operands[0].imm
                    if start<=target<switch:pending.append(target)
                    else:exits.append({'site':hex(pc),'target':hex(target)})
                else:exits.append({'site':hex(pc),'target':'indirect; explicit switch tables traced separately, other targets not assumed'})
                if ins.mnemonic=='jmp':break
            if ins.mnemonic in ('ret','int3','ud2'):break
            pc+=ins.size
    xs=sorted(decoded.values(),key=lambda i:i.address)
    return {'entry':hex(start),'switch_table':hex(switch),
            'states':[hex(x) for x in targets],'nested_switches':nested,
            'switch_sha256':hashlib.sha256(im.code[offset:offset+count*4]).hexdigest(),
            'reachable_instructions':len(xs),'unavailable_reachable':failures,
            'exits':exits,'instructions':[im.format(i) for i in xs]}

def main():
    im=CodeImage()
    rows=[trace(im,0x24c4e0,0x24c71c),trace(im,0x25da80,0x25e0a8),
          trace(im,0x24c740,0x24d204,24,[(0x24c8da,0x24d264,7)])]
    result={'status':'reachable_startup_state_instructions_recovered_not_hooked',
            'source':'authenticated saved PC code and validity mask; switch tables read from decrypted saved text, not packed on-disk EXE',
            'exe_sha256':im.meta['exe_sha256'],'functions':rows,
            'limit':'Does not establish whole startup ordering, GUI readiness or language initialization. No native execution or game changes.'}
    # Cross-check actual class tables and language initialization in aLogo.
    assert im.pointer(0x2f0f3d8)==0x24c4e0
    assert im.pointer(0x2f0f3e8)==0x24c740
    assert im.pointer(0x2f108d8)==0x25da80
    logo={int(line.split(':',1)[0],16):line for line in rows[2]['instructions']}
    assert 'byte ptr [rax + 0x108]' in logo[0x24d040]
    assert 'ecx, 0x18' in logo[0x24d047]
    assert 'RIP target +51C4640' in logo[0x24d04c]
    assert 'dword ptr [rax + 0x4c], ecx' in logo[0x24d053]
    result['language_witness']={
        'class':'aLogo; object vtable2F0F3B0, update virtual38 at24C740',
        'read':'24D039 loads manager5013950;24D040 reads byte+108',
        'bound':'24D047 checks index below24 before writing',
        'write':'24D04C loads GUI51C4640;24D053 writes current text language+4C',
        'consumer':'24D06A/24D0CC call original22AB570 text-path rewrite in mode0',
        'scope':'Language initialization is statically connected in startupLogo. Exact insertion phase and title transition hold still pending.'}
    assert 'dword ptr [rcx + 0xa0]' in logo[0x24cf1f]
    assert 'RIP target +4C981A8' in logo[0x24cf81]
    assert 'RIP target +51C42B8' in logo[0x24cfb8]
    assert 'call     qword ptr [rax + 0x50]' in logo[0x24cfc2]
    result['title_transition_witness']={
        'stage':'aLogo state+50==22; inner state+A0==2',
        'type_registration':'6B00 registers aTitle under4C981A8;25C8C0 returns same type descriptor',
        'request':'24CF81 loads aTitle descriptor;24CFB8 gets app manager51C42B8;24CFC2 calls its virtual50',
        'ordering':'state22 inner0 initializes language and requests text;inner1 waits1B80F10 then writes inner2;inner2 requests aTitle',
        'candidate_gate':'Before executing original aLogo update in state22/inner2; resume original only after notice finishes',
        'remaining':'Native message-manager ready/queue availability at this phase and live presentation still need integration/acceptance.'}
    result['limit']='Title-request phase and language ordering connected statically. Native message availability and production hook integration not yet completed; no game changes.'
    (P/'research/horizon-complete-notice-pretitle-cfg-20261021.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    lines=[]
    for row in rows:lines.extend(['; '+row['entry'],*row['instructions']])
    (P/'research/horizon-complete-notice-pretitle-cfg-20261021.asm').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print([(r['entry'],r['reachable_instructions'],r['unavailable_reachable']) for r in rows])

if __name__=='__main__':main()
