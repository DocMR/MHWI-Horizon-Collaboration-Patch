"""Capture native armor reload/resource state; read-only, no remote calls."""
import ctypes as c,hashlib,json,struct,time
from pathlib import Path
from trace_live_quest_readonly import k,find_game,modules,FIELDS
ROOT=Path(__file__).resolve().parents[1]

def main():
    pid=find_game()
    if pid is None:raise RuntimeError('Game not running')
    mods=modules(pid);game=next(x for x in mods if x['name'].lower()=='monsterhunterworld.exe')
    expected=json.loads((ROOT/'research/exported-text-metadata.json').read_text(encoding='utf-8'))['exe_sha256']
    assert hashlib.sha256(Path(game['path']).read_bytes()).hexdigest()==expected
    handle=k.OpenProcess(0x1010,False,pid)
    if not handle:raise c.WinError(c.get_last_error())
    def read(a,n):
        b=c.create_string_buffer(n);got=c.c_size_t()
        if not k.ReadProcessMemory(handle,a,b,n,c.byref(got)) or got.value!=n:raise c.WinError(c.get_last_error())
        return b.raw
    def q(a):return struct.unpack('<Q',read(a,8))[0]
    def block(a,n):
        try:return dict(address=hex(a),raw=read(a,n).hex()) if a else dict(address='0x0')
        except OSError as e:return dict(address=hex(a),error=str(e))
    try:
        base=game['base'];manager=q(base+0x500eca0);identity=q(manager+0xae40)
        slots=[];actor=0;slot_base=0
        for slot in range(24):
            entry=manager+slot*0x740
            ids=struct.unpack('<2i',read(entry+0x50,8))
            if any(v>=0 for v in ids):
                row=dict(slot=slot,identity=hex(q(entry+0x48)),actor=hex(q(entry+0x58)),ids=ids)
                slots.append(row)
                if q(entry+0x48)==identity:actor=q(entry+0x58);slot_base=entry
        assert actor and read(actor+0xc,1)[0]&14
        samples=[]
        for _ in range(3):
            reload=q(actor+0x12610);face=q(actor+0x126c8)
            resource_manager=q(base+0x50137c8)
            tasks=[]
            for event in range(24):
                job=struct.unpack('<i',read(slot_base+0x78+event*4,4))[0]
                item=dict(event=event,job=job)
                if 0<=job<1024:
                    at=resource_manager+job*40
                    item.update(state=struct.unpack('<I',read(at+0x4c,4))[0],raw=read(at+0x30,40).hex())
                    pool=q(at+0x38);resources=[];seen=set()
                    while pool and pool not in seen and len(seen)<8:
                        seen.add(pool);count=struct.unpack('<I',read(pool+0xc00,4))[0]
                        if count>32:break
                        for index in range(count):
                            data=read(pool+index*96,96);pointer=struct.unpack_from('<Q',data)[0]
                            row=dict(path=data[16:96].split(b'\0',1)[0].decode('ascii',errors='replace'),pointer=hex(pointer),raw=data.hex())
                            if pointer:
                                header=block(pointer,0xb8);row['resource']=header
                                if 'raw' in header:
                                    raw_header=bytes.fromhex(header['raw']);row['resource_name']=raw_header[12:0x90].split(b'\0',1)[0].decode('ascii',errors='replace')
                            resources.append(row)
                        pool=q(pool+0xc08)
                    item['resources']=resources
                tasks.append(item)
            samples.append(dict(time_ns=time.time_ns(),actor=hex(actor),
                appearance=block(actor+0x13690,0x200),actor_flags=block(actor+0x12610,0xd0),
                reload=block(reload,0x300),face_controller=block(face,0x480),
                actor_resource_entry=block(slot_base,0x740),
                component_pointers={hex(o):hex(q(actor+o)) for o in [0x12610,0x126c8,0x126d8,0x468]},
                resource_manager=block(resource_manager,0x300),tasks=tasks,
                face_resource=block(q(face+0x3f0),0x128)))
            time.sleep(.25)
        quest=q(base+0x500ed30);raw=read(quest,0x29462) if quest else None
        state={name:struct.unpack_from('<'+fmt,raw,off)[0] for name,(off,fmt) in FIELDS.items()} if raw else {}
        report=dict(pid=pid,base=hex(base),modules=mods,actor_slots=slots,samples=samples,quest=state,
                    memory_writes=False,remote_calls=False,save_access=False,
                    limits=['Non-atomic raw native state; task IDs and pending fields require authenticated consumer interpretation.'])
        out=ROOT/'research'/('horizon-armor-live-'+time.strftime('%Y%m%d-%H%M%S')+f'-{pid}.json')
        out.write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps(dict(path=str(out),pid=pid,actor=hex(actor),reload=hex(reload),quest=state,memory_writes=False)))
    finally:k.CloseHandle(handle)

if __name__=='__main__':main()
