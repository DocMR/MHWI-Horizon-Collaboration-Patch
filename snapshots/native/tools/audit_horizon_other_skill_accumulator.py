"""Recover skill accumulator code separately from its certified inline switches."""
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P
from capstone.x86 import X86_OP_IMM

def main():
 im=CodeImage();a,z=0x12F82B0,0x12F8957
 payload=im.code[a-im.base:z-im.base];assert 0 not in im.valid[a-im.base:z-im.base]
 body=list(im.md.disasm(payload,a));assert sum(i.size for i in body)==len(payload)
 by={i.address:i for i in body};assert body[-1].mnemonic=='ret'
 for i in body:
  if i.mnemonic.startswith('j') and i.operands[0].type==X86_OP_IMM:
   assert i.operands[0].imm in by, (hex(i.address),i.op_str)
 switches=[]
 for table,selectors,default in [(0x12F8958,0x12F8974,0x12F8729),(0x12F89FC,0x12F8A18,0x12F8862)]:
  targets=struct.unpack_from('<7I',im.code,table-im.base)
  indices=im.code[selectors-im.base:selectors-im.base+134]
  assert len(indices)==134 and max(indices)==6 and targets[6]==default
  assert all(t in by for t in targets)
  switches.append({'jump_table':hex(table),'selector_table':hex(selectors),
     'targets':[hex(t) for t in targets],'selectors':list(indices),
     'input_range':[11,144],'id209_route':'outside range, direct default',
     'sha256':hashlib.sha256(im.code[table-im.base:selectors-im.base+134]).hexdigest()})
 checks={0x12F86B5:'ecx, word ptr [rax + 8]',0x12F8729:'eax, byte ptr [r8 + 0xa]',
   0x12F873E:'dword ptr [r10 + 0x84], esi',0x12F8747:'esi, 0x1f5',
   0x12F890E:'edx, word ptr [rax + 8]',0x12F8933:'dword ptr [rbx + rcx*8 + 0x88], edi'}
 for addr,s in checks.items():assert by[addr].op_str==s
 assert by[0x12F8730].mnemonic=='jl' and by[0x12F8737].mnemonic=='jle'
 report={'status':'accumulator_code_and_inline_tables_separated_native_selection_verified',
   'exe_sha256':im.meta['exe_sha256'],'code_range':[hex(a),hex(z)],
   'sha256':hashlib.sha256(payload).hexdigest(),'instruction_count':len(body),
   'instructions':[im.format(i) for i in body],'switches':switches,
   'selection':'Scans501 native records via462700; record+8 selects skill-point slot; signed count controller+80+24*ID compared with signed threshold record+0A. If met and threshold exceeds current selected threshold, stores record ordinal ESI at controller+84+24*ID.',
   'secondary_selection':'Second scan uses count+81 and stores record ordinal at+88.',
   'id209':'209 outside both11..144 switches: no extra controller69..71 count adjustment in these dispatches.',
   'terminology_correction':'12F80F0 reads selected native record ordinal at+84, not literal skill level or point count. Existing getter bytes remain correct; prior level labels were imprecise.',
   'limits':['Serialized35-byte record to runtime+8/+0A mapping not yet certified; do not infer threshold5 solely from raw record467.',
    'Equipment reader1671510 and snapshot field binding remain to certify.',
    'One initial mode gate1B7C790 and deferred-record prerequisite path remain part of native behavior.',
    'This verifies record selection, not downstream Shield-Weaver effect execution.',
    'Static disassembly only; no target execution or game-directory writes.']}
 (P/'research/horizon-other-skill-accumulator.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print(json.dumps({'instructions':len(body),'switches':2,'native_records_scanned':501,'id209_special_adjustment':False,'effect_complete':False}))
if __name__=='__main__':main()
