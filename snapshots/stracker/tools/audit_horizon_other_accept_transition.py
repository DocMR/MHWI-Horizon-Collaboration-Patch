"""Record native consumers of accept callback state without replacing checks."""
import json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();functions=[]
for root in (0x113F480,0x113A430,0x1B59CF0,0x1B599A0):
 fs,xs=decode_group(im,root)
 functions.append({'root':hex(root),'fragments':[list(f) for f in fs],'instructions':[im.format(i) for i in xs]})
by={i.address:i for i in decode_group(im,0x113F480)[1]}
for a,raw in {0x113F5D0:'4038afa82b0000',0x113F5D9:'4038afa42b0000',0x113F605:'83bc8f2829000004',0x113F616:'488d15535c0e02',0x113F62F:'c687a42b000001'}.items():assert by[a].bytes.hex()==raw
at=im.raw(0x3225270);assert im.exe[at:at+14]==b'snd_q_approve\0'
report={'exe_sha256':im.meta['exe_sha256'],'callback_field':'0x2BA8','consumer':'0x113F480',
 'observed_transition':'A nonzero callback field with zero +2BA4 proceeds through native session checks; a per-participant +2928 status4 branch requests snd_q_approve; +2BA4 is then set to1.',
 'sound_key':'snd_q_approve','sound_key_rva':'0x3225270','functions':functions,
 'remaining':['Prove callback receiver and consumer receiver are the same class instance through actual dispatch.','Trace rank condition execution separately; participant/session status helpers are not HR/MR checks.'],
 'limits':['Explicit-offset matches elsewhere are only candidates, not same-object proof.','No native checks bypassed, no target code executed.']}
(P/'research/horizon-other-accept-transition.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'complete_groups':len(functions),'sound_key':report['sound_key'],'rank_predicate_proven':False}))
