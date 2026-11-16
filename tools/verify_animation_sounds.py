"""Read-only original animation/audio routing oracle, not an audible runtime test.

Uses real collection sequence metadata and original Build 434 machine code.
Only collection lookup and final audio mixer are stubbed; frame advancement,
visibility gating and object-position routing execute original instructions.
"""
import json, struct
from types import SimpleNamespace
from level_assets import inventory, raw_tag, ROOT
import myth_collection
from verify_combat_builders import machine, run, STOP, u
from unicorn.x86_const import *

tags = inventory()
sound_ids = {key: i for i, (kind,key) in enumerate(k for k in tags if k[0]=='soun')}
objects, sequence, lookup, visibility = (STOP+x for x in (0x20000,0x21000,0x22000,0x23000))

def sequences(key):
    data=bytes(60)+b'mth2'+raw_tag(tags,'.256',key)
    head=myth_collection.parse_collection_header(data,SimpleNamespace(version=0,signature='mth2',tag_data_offset=64))
    return myth_collection.parse_sequences(data,head)

def simulate(meta,visible=True):
    c=machine(); events=[]
    c.mem_write(0x52a9f4,struct.pack('<I',objects))
    c.mem_write(objects+8,struct.pack('<iii',512,512,0))
    c.mem_write(objects+0x30,bytes([meta.ticks_per_frame]))
    c.mem_write(lookup+0x48,struct.pack('<I',sequence))
    c.mem_write(sequence+10,struct.pack('<7h',meta.frames_per_view,meta.ticks_per_frame,meta.key_frame,meta.loop_frame,
        *[sound_ids.get(getattr(meta,'sound_tag_'+p).decode('latin1'),-1) for p in ('first','key','last')]))
    c.mem_write(0x67ec64,struct.pack('<h',16));c.mem_write(0x67ec62,struct.pack('<h',16))
    c.mem_write(0x67ec58,struct.pack('<I',visibility))
    c.mem_write(visibility+4,struct.pack('<I',4 if visible else 0))
    def hook(cpu,address,size,user):
        if address not in (0x4699ba,0x4cbb47):return
        esp=cpu.reg_read(UC_X86_REG_ESP)
        ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
        if address==0x4699ba:cpu.reg_write(UC_X86_REG_EAX,lookup)
        else:
            slot,position,volume,flags=struct.unpack('<4I',cpu.mem_read(esp+4,16))
            assert slot==0 and position==objects+8 and volume==256
            events.append(cpu.reg_read(UC_X86_REG_EAX))
        cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
    c.hook_add(u.UC_HOOK_CODE,hook)
    for _ in range(meta.frames_per_view*meta.ticks_per_frame*2):run(c,0x44db8a,[0],{})
    return events

def main():
    rows=[]
    for collection in ('wam2','warr','jom1','trow','wigh','chic','wolf'):
        for index,seq in enumerate(sequences(collection)):
            m=seq['metadata']
            if not 0<m.ticks_per_frame<128 or not 0<m.frames_per_view<128:continue
            assigned={p:getattr(m,'sound_tag_'+p).decode('latin1') for p in ('first','key','last')}
            present={p:t for p,t in assigned.items() if t in sound_ids}
            events=simulate(m)
            assert not simulate(m,False), (collection,index,'hidden sound emitted')
            if not present:assert not events,(collection,index,events)
            for tag in present.values():assert sound_ids[tag] in events,(collection,index,tag,events)
            rows.append(dict(collection=collection,index=index,name=str(seq['name']),sounds=present,
                emitted_sound_tags=[next(k for k,v in sound_ids.items() if v==e) for e in events],
                hidden_emits=False))
    warrior={x['name']:x for x in rows if x['collection']=='wam2'}
    for name in ('walk','stand 1','stand 2'):assert not warrior[name]['emitted_sound_tags']
    for name in ('flinch','celebration','taunt'):assert 'wacl' in warrior[name]['emitted_sound_tags']
    out=ROOT/'research/reports/animation-sounds-verification.json'
    out.write_text(json.dumps(rows,ensure_ascii=True,indent=2)+'\n',encoding='utf-8')
    print('PASS',len(rows),'real sequences: original frame timing, audio routing and hidden gating')
    print('Warrior: idle/walk silent; flinch/celebration/taunt emit wacl. No new playback layer required.')

if __name__=='__main__':main()
