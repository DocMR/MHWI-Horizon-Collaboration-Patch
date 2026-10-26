"""Static skill-controller initialization and refresh caller evidence."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage(); methods={};by={}
 for entry in [0x12F7CE0,0x12F8150]:
  fragments,body=decode_group(im,entry);assert body
  by[entry]={i.address:i for i in body}
  methods[hex(entry)]={'fragments':[list(f) for f in fragments],
    'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
    'instructions':[im.format(i) for i in body]}
 b=by[0x12F7CE0]
 assert b[0x12F7CF5].op_str=='edx, 0xe2' and b[0x12F7D1C].op_str=='rax, [rax + 0x18]'
 assert b[0x12F7D49].op_str=='rax, [rax + 0x18]'
 b=by[0x12F8150]
 assert b[0x12F81D2].op_str=='rcx, qword ptr [rsi + 0x60]'
 assert b[0x12F81DB].op_str=='0x19a7450' and b[0x12F81E8].op_str=='0x12f82b0'
 try: decode_group(im,0x12F82B0); complete=True
 except AssertionError: complete=False
 report={'status':'controller_refresh_entry_verified_accumulator_unresolved','exe_sha256':im.meta['exe_sha256'],
  'methods':methods,'controller_record_count':226,'record_stride':24,'first_record':120,
  'candidate209':{'record_start':120+24*209,'selected_record_field':132+24*209},
  'refresh':'12F8150 tests controller+5C bit1 and owner+1261C bit2/controller+68; builds equipment snapshot using19A7450(owner,stack); passes snapshot to12F82B0(controller,stack).',
  'accumulator_group_decodable':complete,
  'limits':['Not yet certified that armor serialized209/1 reaches this accumulator correctly.',
   'Generic group decoding remains false because inline tables followRET; see horizon-other-skill-accumulator.json for409-instruction bounded code and both switches; no patch at this entry.',
   'No evidence here of defense/reaction/depletion/recovery effects.',
   'Native code not executed; files remain in project directory.']}
 (P/'research/horizon-other-skill-refresh-entry.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('Skill-controller226x24 initialization and refresh caller verified; accumulation/effect still pending.')
if __name__=='__main__':main()
