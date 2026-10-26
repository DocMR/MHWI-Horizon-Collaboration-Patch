"""Static deferred pending-mask completion and skill refresh notification."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage();methods={};maps={}
 for a in [0x1257E30,0x1258C10]:
  f,b=decode_group(im,a);assert b
  maps[a]={i.address:i for i in b}
  methods[hex(a)]={'fragments':[list(v) for v in f],
   'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in b)).hexdigest(),
   'instructions':[im.format(i) for i in b]}
 b=maps[0x1257E30]
 checks={0x1258293:'eax, edi',0x1258296:'dword ptr [rbx + 0x2ac], eax',
  0x1258426:'byte ptr [rbx + 0x2a0], 0',0x1258433:'dword ptr [rbx + 0x2ac], 0',
  0x125848C:'eax, 2',0x1258490:'dword ptr [rcx + 0x1261c], eax',
  0x125849C:'eax, 2',0x12584A0:'dword ptr [rcx + 0x12630], eax',
  0x12584C7:'byte ptr [rbx + 0x2a0], al'}
 for a,s in checks.items():assert b[a].op_str==s
 assert b[0x125842D].op_str=='0x12584bd' and b[0x125843A].op_str=='0x12584c4'
 q=maps[0x1258C10]
 assert q[0x1258C3F].op_str=='0x1b42ed0' and q[0x1258C4E].op_str=='eax, edi'
 report={'status':'deferred_pending_zero_transition_requests_native_skill_recompute',
  'exe_sha256':im.meta['exe_sha256'],'methods':methods,
  'completion':'1257E30 clears pending2AC bits under original queried conditions. If prior2A0 wasnonzero and now2AC iszero, notification block setsbit3 andbit2 inowner1261C/12630, sends original notification1B42F40 then stores2A0=(2AC!=0).',
  'helper':'1258C10 returns0 when target bitnotset,1 when predicate1B42ED0 false,2 after predicate true and bit cleared. Predicate semantics retained asunknown.',
  'relationship':'Complements immediate1258800 maskzero notification.12F8150 tests the sameowner1261C bit2 before snapshot+accumulation.',
  'limits':['All pending predicates are retained; their individual semantics not certified as asset readiness.',
   'Controller concrete identity and scheduling/order with skill callback remain to certify.',
   'No claim that every lifecycle or remote actor uses these paths.',
   'Shield-Weaver downstream effects remain incomplete; no target execution or game-directory writes.']}
 (P/'research/horizon-other-skill-pending-completion.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('404-instruction pending update plus bit-clear helper verified; zero transition requests skill recompute; scheduling pending.')
if __name__=='__main__':main()
