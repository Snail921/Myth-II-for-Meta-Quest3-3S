"""Verify facing-bearing movement against the user's original single/multi-unit Films."""
from verify_waypoint_input import *
for name in ['Move Stop with Direction','Move Waypoint multiUnit']:
 film=json.loads((ROOT/'research/reports/combat-films'/f'{name}.json').read_text(encoding='utf-8'))
 cmd=next(x for x in film['commands'] if x['verb']==3 and x['movement_candidate']['flags']&16)
 move=cmd['movement_candidate'];ids=move['unit_ids'];points=move['waypoints']
 c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES));c.mem_write(BYTES,b'\xa5'*2048)
 c.mem_write(0x553fd8,struct.pack('<HH',len(ids),0));c.mem_write(0x553fdc,struct.pack('<'+'H'*len(ids),*range(len(ids))))
 for i,id in enumerate(ids):c.mem_write(ACTORS+i*216,struct.pack('<H',id))
 for j,p in enumerate(points):
  facing=(0x10000|move['final_facing_raw']) if j==len(points)-1 else 0
  assert send(3 if len(points)==1 else 13,struct.pack('<5I',0,int(j==0 and len(points)>1),p['raw_x'],p['raw_y'],facing))==1
 if len(points)>1:assert send(14,bytes(20))==1
 size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2]
 assert bytes(c.mem_read(BYTES+8,size-8))==bytes.fromhex(cmd['payload_hex']),name
 print('PASS original Film facing payload:',name)
assert send(3,struct.pack('<5I',0,0,95000,78000,123))==16
assert send(15,struct.pack('<5I',0,0,95000,78000,0x20000))==16
print('PASS malformed facing rejection')
