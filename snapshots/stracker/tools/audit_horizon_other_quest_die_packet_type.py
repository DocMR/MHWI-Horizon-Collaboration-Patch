"""Authenticate the concrete packet type feeding native quest goal notifications."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P


def main():
    im=CodeImage()
    registration=im.instructions(0x138220)
    r={i.address:i for i in registration}
    assert im.target(r[0x138231])==0x31D6358
    off=im.raw(0x31D6358)
    name=im.exe[off:off+100].split(b'\0')[0].decode('ascii')
    assert name=='cPacketEmQuestDie'
    assert im.target(r[0x13823C])==0x4F827B8
    assert r[0x138247].op_str=='r9d, [rax + 0x28]'
    assert im.target(r[0x138250])==0x31D6338
    assert im.pointer(0x31D6338+8)==0xF36D00
    assert im.pointer(0x31D6338+0x10)==0xF36D90
    methods={}
    for entry in (0xF36D00,0xF36D90):
        body=im.instructions(entry);assert body
        methods[entry]={i.address:i for i in body}
    f=methods[0xF36D00];p=methods[0xF36D90]
    assert im.target(f[0xF36D58])==0x31D6370
    assert im.target(f[0xF36D65])==0x4F827EC==0x4F827B8+0x34
    assert f[0xF36D6B].op_str=='dword ptr [rbx + 8], ecx'
    assert f[0xF36D72].op_str=='byte ptr [rbx + 0x20], 0'
    assert im.target(p[0xF36DA6])==0x31D6370
    assert im.target(p[0xF36DB0])==0x4F827EC
    assert p[0xF36DB6].op_str=='dword ptr [rbx + 8], eax'
    assert p[0xF36DC0].op_str=='byte ptr [rbx + 0x20], 0'
    dispatch=im.instructions(0x16B8870);d={i.address:i for i in dispatch}
    assert im.target(d[0x16B95ED])==0x4F827EC
    assert d[0x16B95F6].mnemonic=='cmove' and d[0x16B95F6].op_str=='rsi, r8'
    assert d[0x16B95FD].op_str=='0x16b9686'
    assert d[0x16B9650].op_str=='ebx, byte ptr [rsi + 0x20]'
    assert d[0x16B9661].op_str=='0x1cc2d90'
    # The discriminator originates at the common packet header; the existing
    # comparison chain reaches this type only when preceding types did not match.
    assert d[0x16B8884].op_str=='eax, dword ptr [r8 + 8]'
    report={
        'status':'cPacketEmQuestDie_registration_constructors_and_consumer_discriminator_certified',
        'exe_sha256':im.meta['exe_sha256'],'name':name,'name_rva':'0x31d6358',
        'dti':'0x4f827b8','dti_vtable':'0x31d6338','instance_vtable':'0x31d6370',
        'object_size':0x28,'dynamic_type_id':'DTI+34=4F827EC -> packet+8 -> dispatcher comparison16B95ED',
        'registration':[im.format(i) for i in registration],
        'constructors':{hex(entry):{'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in rows.values())).hexdigest(),
            'instructions':[im.format(i) for i in rows.values()]} for entry,rows in methods.items()},
        'dispatch_window':[im.format(i) for i in dispatch if 0x16B95ED<=i.address<0x16B9686],
        'connection':'Authenticated EmQuestDie packet entity reference+18 and event byte+20 feed the native entity-goal notification path documented in horizon-other-quest-entity-goal-notification.json.',
        'implementation_decision':'Retain the original packet type, discriminator and handling. This module does not introduce a duplicate quest-death packet or new wire layout.',
        'limits':['Concrete type name is now certified, but byte20 values0/1 gameplay meanings and all senders remain to audit.',
            'Factory initializes byte20 to zero; do not infer that later packets can never carry1.',
            'Registration/type identity does not prove all local/remote authority and reliable-delivery behavior.',
            'Static code/data only; no packet sent, target execution, dynamic test or game-directory write.']}
    (P/'research/horizon-other-quest-die-packet-type.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Concrete cPacketEmQuestDie type and DTI34 -> packet8 -> native dispatcher/goal notification identity certified.')


if __name__=='__main__':main()
