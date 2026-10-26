"""Static COFF/ABI audit of the emitted reaction bridge; never load/run it."""
import hashlib,json,struct
from analyze_hzd_code import P
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
path=P/'staging/Horizon-other-module/build/link-only/horizon_shield_reaction_entry_asm.obj'
data=path.read_bytes();machine,count=struct.unpack_from('<HH',data);assert machine==0x8664
optional=struct.unpack_from('<H',data,16)[0];sections={}
for n in range(count):
 o=20+optional+n*40;name=data[o:o+8].split(b'\0')[0].decode();size,offset=struct.unpack_from('<II',data,o+16)
 sections[name]=data[offset:offset+size]
code=sections['.text$mn'];unwind=sections['.xdata'];md=Cs(CS_ARCH_X86,CS_MODE_64);b=list(md.disasm(code,0))
assert sum(i.size for i in b)==len(code) and b[-1].mnemonic=='ret'
ops=[(i.mnemonic,i.op_str) for i in b]
assert ops[:3]==[('push','rbp'),('lea','rsp, [rsp - 0xd0]'),('lea','rbp, [rsp + 0xd0]')]
assert ('mov','rcx, r11') in ops and ('mov','byte ptr [rsp + 0xc8], al') in ops
assert ops[-6:]==[('xor','r15b, r15b'),('mov','r15b, byte ptr [rsp + 0xc8]'),('mov','ebx, dword ptr [rbx + 0x10]'),('lea','rsp, [rbp]'),('pop','rbp'),('ret','')]
assert len([i for i in b if i.mnemonic=='call'])==1
assert ('pushfq','') in ops and ('popfq','') in ops
assert unwind[:4]==bytes.fromhex('011104d5') and len(unwind)==12
for n,off in enumerate([0x60,0x70,0x80,0x90,0xa0,0xb0]):
 assert ('movdqu',f'xmm{n}, xmmword ptr [rsp + 0x{off:x}]') in ops
assert not any(i.op_str.startswith('xmm6,') for i in b)
report={'status':'emitted_coff_static_abi_checks_passed_not_execution','object_sha256':hashlib.sha256(data).hexdigest(),
 'code_bytes':len(code),'instruction_count':len(b),'unwind_raw':unwind.hex(),
 'stack':'entryRSP8mod16; pushRBP ->0mod16; allocateD0 ->0mod16; leaves32-byte shadow space',
 'result':'R15b only; restore original RAX and volatile registers; original XOR flags retained after byte result load',
 'replay':'XOR R15b,R15b; MOV R15b,[saved bool]; MOV EBX,[RBX+10] before legal frame-pointer epilogue',
 'instructions':[f'{i.address:04X}: {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in b],
 'limits':['Static emitted-byte and frame header checks, not OS unwind execution or native equivalence test.','No target execution or game-directory writes.']}
(P/'research/horizon-other-shield-reaction-assembly.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('Emitted reaction COFF, local byte result, replay tail and frame header statically verified; not executed.')
