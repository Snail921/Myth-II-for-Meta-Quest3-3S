"""Unmodified original ring-parameter arithmetic at exact simulation ticks.
Viewport acceptance and final renderer are stubbed, not the effect calculation.
"""
from verify_combat_builders import *
c=machine();effect=0x100d000;definition=0x100e000
c.mem_write(0x529c1e,struct.pack('<H',1));c.mem_write(0x529c24,struct.pack('<I',effect));c.mem_write(0x529cbc,struct.pack('<I',definition))
c.mem_write(effect,struct.pack('<H',1));c.mem_write(effect+8,struct.pack('<hhI',12,24,100))
c.mem_write(definition,struct.pack('<I',2));c.mem_write(definition+6,struct.pack('<h',1024));c.mem_write(definition+10,struct.pack('<hh',51,0));c.mem_write(definition+26,struct.pack('<h',512));c.mem_write(0x67f2f4,bytes(4))
observed=[]
def hook(cpu,address,size,user):
 if address not in (0x4b950a,0x436b42):return
 sp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(sp,4))[0]
 if address==0x4b950a:cpu.reg_write(UC_X86_REG_EAX,1)
 else:
  view,point,radius,height,fade=struct.unpack('<5I',cpu.mem_read(sp+4,20));xy=struct.unpack('<2i',cpu.mem_read(point,8));observed.append((*xy,radius,height,65535-fade))
 cpu.reg_write(UC_X86_REG_ESP,sp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,hook);rows=[]
for age in range(24):
 c.mem_write(effect+16,struct.pack('<I',age));run(c,0x436a50,[0,0],{})
 rows.append(struct.pack('<6I',age,*observed[-1]))
(ROOT/'build/shockwave-oracle.bin').write_bytes(struct.pack('<I',len(rows))+b''.join(rows))
print('PASS original mesh-effect radius, height, opacity oracle:',len(rows),'ticks')
