"""Compiled route staging and assembly thunk against the original movement builder."""
from verify_group_selection import *
from verify_combat_builders import QUEUE,BYTES
film=json.loads((ROOT/'research/reports/combat-films/Move Waypoint.json').read_text())
cmd=next(x for x in film['commands'] if x['verb']==3);pts=cmd['movement_candidate']['waypoints']
def formation(cpu,address,size,user):
 if address!=0x468e13:return
 esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
 cpu.reg_write(UC_X86_REG_EAX,0);cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,formation)
c.mem_write(0x52a744,struct.pack('<I',2181));c.mem_write(ACTORS+2*216,struct.pack('<H',10011))
c.mem_write(0x553fd8,struct.pack('<HH',1,0));c.mem_write(0x553fdc,struct.pack('<H',2))
for i,p in enumerate(pts):
 assert send(13,struct.pack('<5I',0,int(i==0),p['raw_x'],p['raw_y'],0))==1
 assert struct.unpack('<7I',c.mem_read(QUEUE,28))[2]==0
assert send(14,bytes(20))==1
size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2]
assert bytes(c.mem_read(BYTES+8,size-8))==bytes.fromhex(cmd['payload_hex'])
assert send(14,bytes(20))==19
assert send(13,struct.pack('<5I',0,1,1000,1000,0))==1
c.mem_write(0x553fdc,struct.pack('<H',3))
assert send(14,bytes(20))==19
print('PASS staged four-point route matches original Film; no early movement; duplicate commit and changed selection rejected')
