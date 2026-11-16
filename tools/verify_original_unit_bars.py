"""Execute original Build 434 frame projection and mana bar builder."""
import struct
from verify_combat_builders import machine,run,STOP,STACK,ROOT
from unicorn.x86_const import *
c=machine();obj=STOP+0x21000;actor=STOP+0x22000;definition=STOP+0x23000;viewer=STOP+0x24000;buffer=STOP+0x25000;frame=STACK-0x100
c.mem_write(0x529f54,struct.pack('<I',actor));c.mem_write(0x52a6f4,struct.pack('<I',definition));c.mem_write(obj+0x28,b'\x02');c.mem_write(obj+0x26,bytes(2));c.mem_write(definition+0x214,struct.pack('<h',25600));c.mem_write(0x4c7cb0,b'\xc3')
for current in (0,6400,12800,25600,30000):
 c.mem_write(actor+0x7a,struct.pack('<h',current));c.mem_write(0x67563c,struct.pack('<I',buffer));c.mem_write(0x675634,struct.pack('<I',buffer+256));c.mem_write(buffer,bytes(128))
 run(c,0x4bfc7b,[viewer,0,10,100,0,0,65535],{UC_X86_REG_EAX:obj,UC_X86_REG_ECX:110,UC_X86_REG_EBX:viewer})
 left,top,right,bottom=struct.unpack('<4h',c.mem_read(buffer+0x18,8));assert left==10 and right-left==min(current,25600)*100//25600,(current,left,right)
assert c.mem_read(buffer+0x38,8)==bytes.fromhex('00000000ffff0000')
for flags in (0,0x40000):
 c.mem_write(obj+0x20,struct.pack('<3h',140,-20,650));c.mem_write(frame+4,struct.pack('<ii',100,200));c.mem_write(frame+0x40,struct.pack('<f',.5));c.mem_write(viewer+0x7a,struct.pack('<hh',640,480))
 c.reg_write(UC_X86_REG_EBP,frame);c.reg_write(UC_X86_REG_EBX,obj);c.reg_write(UC_X86_REG_EDI,viewer);c.reg_write(UC_X86_REG_ESI,flags);c.reg_write(UC_X86_REG_ESP,STACK)
 c.emu_start(0x4bf296,0x4bf335,count=10000)
 a,b=[struct.unpack('<i',c.mem_read(frame+x,4))[0] for x in (0x30,0x44)];t,bt=[struct.unpack('<i',c.mem_read(frame+x,4))[0] for x in (0x28,0x48)]
 assert abs(a-b)==140 and abs(bt-t)==335,(flags,a,b,t,bt)
print('PASS original MP builder: 0/25/50/100/clamped, fixed blue; frame: radius and vertical extents, inverted flag')
