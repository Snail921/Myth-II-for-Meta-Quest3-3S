"""Compiled pickup hook + unmodified original eligibility and command builder.
Synthetic actor/item definitions exercise ammo pickup; OS, control/voice endpoints
are stubbed by the selection harness. This is not a walk/acquire gameplay test.
"""
from verify_group_selection import *
from verify_combat_builders import QUEUE,BYTES
OBJECTS=0x100c000
DEFS=0x102a000
ITEMS=0x102b000
c.mem_write(0x52a9ee,struct.pack('<H',1400))
c.mem_write(0x52a9d8,struct.pack('<H',80))
c.mem_write(0x52a6f4,struct.pack('<I',DEFS))
c.mem_write(0x52b184,struct.pack('<I',ITEMS))
c.mem_write(0x52a744,struct.pack('<I',2181))
c.mem_write(ACTORS+2*216+0x10,struct.pack('<h',-1));c.mem_write(ACTORS+3*216+0x10,struct.pack('<h',-1))
c.mem_write(0x553f22,struct.pack('<h',-1))
c.mem_write(DEFS+0x19a,struct.pack('<h',3))
c.mem_write(DEFS+0x8a,struct.pack('<h',-1))
c.mem_write(ITEMS+0xe8,struct.pack('<h',-1))
c.mem_write(ITEMS+0xf0,struct.pack('<h',-1))
c.mem_write(ACTORS+2*216+0xe,struct.pack('<h',-1))
c.mem_write(ACTORS+2*216+0xb0,struct.pack('<h',-1))
c.mem_write(0x553fd8,struct.pack('<HH',1,0));c.mem_write(0x553fdc,struct.pack('<H',2))
film=json.loads((ROOT/'research/reports/combat-films/Dwarf set and pick mines.json').read_text(encoding='utf-8'))
for cmd in film['commands']:
 if cmd['verb']!=6:continue
 d=cmd['semantic_candidate'];slot=d['object_index'];identifier=d['object_id'];obj=OBJECTS+slot*80
 c.mem_write(ACTORS+2*216,struct.pack('<H',d['unit_ids'][0]))
 c.mem_write(obj,b'\0'*80);c.mem_write(obj,struct.pack('<H',identifier));c.mem_write(obj+40,b'\x04')
 payload=struct.pack('<5I',(slot<<16)|identifier,0,0,0,0)
 queue=bytes(c.mem_read(QUEUE,28));actors=bytes(c.mem_read(ACTORS,127*216));objects=bytes(c.mem_read(obj,80));v=len(voice)
 hook_state=bytes(c.mem_read(base,dll.OPTIONAL_HEADER.SizeOfImage))
 assert send(16,payload)==101
 assert bytes(c.mem_read(base,dll.OPTIONAL_HEADER.SizeOfImage))==hook_state
 assert bytes(c.mem_read(QUEUE,28))==queue and bytes(c.mem_read(ACTORS,127*216))==actors and bytes(c.mem_read(obj,80))==objects and len(voice)==v
 c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES));c.mem_write(BYTES,b'\xa5'*2048)
 assert send(17,payload)==1
 size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2]
 assert bytes(c.mem_read(BYTES+8,size-8))==bytes.fromhex(cmd['payload_hex'])
 print('PASS compiled PICK_UP + original predicate matches Film tick',cmd['tick'])
# Held/unavailable flag, stale identifier, out of bounds, no selection, mixed eligibility.
queue=bytes(c.mem_read(QUEUE,28))
c.mem_write(obj+4,struct.pack('<I',0x200));assert send(16,payload)==100 and send(17,payload)==18
c.mem_write(obj+4,struct.pack('<I',0));c.mem_write(obj,struct.pack('<H',identifier+1));assert send(16,payload)==100 and send(17,payload)==18
assert send(17,struct.pack('<5I',(8192<<16)|1,0,0,0,0))==18
c.mem_write(obj,struct.pack('<H',identifier));c.mem_write(0x553fd8,struct.pack('<HH',0,0));assert send(16,payload)==100
c.mem_write(0x553fd8,struct.pack('<HH',2,0));c.mem_write(0x553fdc,struct.pack('<HH',2,3))
c.mem_write(ACTORS+3*216+2,struct.pack('<h',1));c.mem_write(DEFS+1024+0x3ca,struct.pack('<h',-1));c.mem_write(DEFS+1024+0x8a,struct.pack('<h',-1));c.mem_write(ACTORS+3*216+0xe,struct.pack('<h',-1));c.mem_write(ACTORS+3*216+0xb0,struct.pack('<h',-1))
assert send(16,payload)==100 and send(17,payload)==18
assert bytes(c.mem_read(QUEUE,28))==queue
# Capacity full: original predicate, not a VR class-specific rule.
c.mem_write(0x553fd8,struct.pack('<HH',1,0));c.mem_write(ACTORS+2*216+0x8c,b'\x03');assert send(16,payload)==100 and send(17,payload)==18
# Identifiers are signed in the original lookup; keep unsigned wire representation.
c.mem_write(ACTORS+2*216+0x8c,b'\0');c.mem_write(obj,struct.pack('<H',0xf123));assert send(16,struct.pack('<5I',(slot<<16)|0xf123,0,0,0,0))==101
assert send(17,struct.pack('<5I',(slot<<16)|0xf123,1,0,0,0))==16
print('PASS read-only query, stale/held/range/capacity/mixed-selection rejection, signed object ID')

# Same original predicate accepts type3 projectile-derived pickups and type4 items.
c.mem_write(0x52af9c,struct.pack('<I',0x102c000));c.mem_write(obj+40,b'\x03')
signed_payload=struct.pack('<5I',(slot<<16)|0xf123,0,0,0,0)
assert send(16,signed_payload)==101
c.mem_write(obj+40,b'\x04');c.mem_write(DEFS+0x3ca,struct.pack('<h',-1));c.mem_write(DEFS+0x8a,struct.pack('<h',7));c.mem_write(ITEMS+0x26,struct.pack('<h',7))
assert send(16,signed_payload)==101
print('PASS original projectile-derived item and matching inventory-definition branches')
