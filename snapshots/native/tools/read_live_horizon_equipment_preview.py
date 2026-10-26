"""Read existing GUI-manager-owned equipment preview objects; no writes/calls."""
import ctypes as c,hashlib,json,struct,time
from pathlib import Path
from trace_live_quest_readonly import k,find_game,modules
ROOT=Path(__file__).resolve().parents[1]

def main():
    pid=find_game()
    if pid is None:raise RuntimeError('Game not running')
    game=next(m for m in modules(pid) if m['name'].lower()=='monsterhunterworld.exe')
    assert hashlib.sha256(Path(game['path']).read_bytes()).hexdigest()==json.loads((ROOT/'research/exported-text-metadata.json').read_text())['exe_sha256']
    handle=k.OpenProcess(0x1010,False,pid)
    if not handle:raise c.WinError(c.get_last_error())
    def read(a,n):
        b=c.create_string_buffer(n);got=c.c_size_t()
        if not k.ReadProcessMemory(handle,a,b,n,c.byref(got)) or got.value!=n:raise c.WinError(c.get_last_error())
        return b.raw
    def q(a):return struct.unpack('<Q',read(a,8))[0]
    try:
        base=game['base'];gui=q(base+0x51c4640);root=read(gui,0x16000)
        matches=[];seen=set()
        for offset in range(0,len(root),8):
            pointer=struct.unpack_from('<Q',root,offset)[0]
            if pointer in seen or not 0x10000<=pointer<0x7fffffffffff:continue
            seen.add(pointer)
            try:
                if q(pointer)!=base+0x346b020:continue
                raw=read(pointer,0xe260);component=raw[0x28f8:0x2958]
                matches.append(dict(gui_slot=hex(offset),object=hex(pointer),raw=raw.hex(),
                                    preview_state=struct.unpack_from('<I',component,8)[0],preview_component=component.hex()))
            except OSError:continue
        preview=q(gui+0x15538)
        preview_raw=read(preview,0x1000) if preview else None
        equipment=q(base+0x5011f00);armor=[]
        for slot in range(5):
            for identity in [225,227,298,299,523,532,718]:
                pointer=q(equipment+0x127e8+8+slot*0x2ef0+identity*8)
                if pointer:
                    data=read(pointer,0x80)
                    armor.append(dict(slot=slot,identity=identity,pointer=hex(pointer),
                                      guard_byte13=data[0x13],raw=data.hex()))
        report=dict(pid=pid,base=hex(base),gui=hex(gui),equipment_boxes=matches,
                    armor_records=armor,
                    preview_manager=dict(address=hex(preview),raw=preview_raw.hex(),
                        state10=struct.unpack_from('<I',preview_raw,0x10)[0],
                        pending_request184_190=list(struct.unpack_from('<4i',preview_raw,0x184)),
                        prior_request1b8_1c4=list(struct.unpack_from('<4i',preview_raw,0x1b8))) if preview else None,
                    read_only=True,remote_calls=False,save_access=False,
                    limits=['Current selected item not inferred from screenshot; match saved native selection fields separately. GUI manager pointer scan only, no heap scan.'])
        out=ROOT/'research'/('horizon-equipment-preview-'+time.strftime('%Y%m%d-%H%M%S')+f'-{pid}.json')
        out.write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps(dict(path=str(out),equipment_boxes=[{k:v for k,v in x.items() if k not in ['raw','preview_component']} for x in matches],preview_manager=hex(preview))))
    finally:k.CloseHandle(handle)

if __name__=='__main__':main()
