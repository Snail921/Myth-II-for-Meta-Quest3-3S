"""Patrol evidence: original hit dispatcher and builder, compiled production hook."""
from verify_waypoint_input import *
# Run original waypoint hit test plus patrol flag branches, not a reimplementation.
for index,flags in [(0,1),(1,3),(2,3)]:
 q=machine();points=[(90000,70000),(93000,70000),(93000,73000)]
 q.mem_write(0x5543ea,struct.pack('<h',3));q.mem_write(0x5543ec,b''.join(struct.pack('<ii',*p) for p in points));q.mem_write(0x553fb0,struct.pack('<ii',*points[index]))
 q.reg_write(UC_X86_REG_ESP,STACK);q.reg_write(UC_X86_REG_EBP,STACK-128);q.reg_write(UC_X86_REG_EDI,0);q.mem_write(STACK-136,bytes(8))
 q.emu_start(0x4671e6,0x46721d,count=20000)
 assert struct.unpack('<H',q.mem_read(STACK-136,2))[0]==flags
 assert struct.unpack('<h',q.mem_read(0x5543ea,2))[0]==3
print('PASS original x86 waypoint hit/flags: first=1, subsequent=3, no duplicate point')
film=json.loads((ROOT/'research/reports/combat-films/Patrol.json').read_text())
for cmd in [x for x in film['commands'] if x['verb']==3 and x['movement_candidate']['flags']==9]:
 move=cmd['movement_candidate'];ids=move['unit_ids'];pts=move['waypoints']
 for mode in (1,2):
  c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES));c.mem_write(BYTES,b'\xa5'*2048)
  c.mem_write(0x553fd8,struct.pack('<HH',len(ids),0));c.mem_write(0x553fdc,struct.pack('<'+'H'*len(ids),*range(len(ids))))
  for i,id in enumerate(ids):c.mem_write(ACTORS+i*216,struct.pack('<H',id))
  for j,p in enumerate(pts):assert send(13,struct.pack('<5I',0,int(j==0),p['raw_x'],p['raw_y'],0))==1
  assert struct.unpack('<7I',c.mem_read(QUEUE,28))[2]==0
  assert send(14,struct.pack('<5I',mode,0,0,0,0))==1
  size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2];expected=bytearray.fromhex(cmd['payload_hex']);expected[1]=9 if mode==1 else 11
  assert bytes(c.mem_read(BYTES+8,size-8))==expected
assert send(14,struct.pack('<5I',3,0,0,0,0))==1
assert send(14,bytes(20))==19
assert send(14,struct.pack('<5I',4,0,0,0,0))==16
print('PASS all 4 recorded loop payloads byte-for-byte, ping-pong uses original verified bits; cancel/invalid/duplicate commit')
# Full four-point route: update final facing without adding a fifth waypoint.
c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES));c.mem_write(BYTES,b'\xa5'*2048)
for j,p in enumerate(pts):assert send(13,struct.pack('<5I',0,int(j==0),p['raw_x'],p['raw_y'],0))==1
p=pts[-1]
assert send(13,struct.pack('<5I',1,0,p['raw_x'],p['raw_y'],0x14000))==1
assert send(14,bytes(20))==1
size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2]
expected=bytearray.fromhex(cmd['payload_hex']);expected[0:2]=struct.pack('>H',24);expected[4:6]=struct.pack('>H',0x4000)
assert bytes(c.mem_read(BYTES+8,size-8))==expected
print('PASS full route final facing replacement, four points preserved')
