"""Compiled 1.8.5 dispatcher + original order builder; no running game writes."""
import sys,struct
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as U
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE('build/integration-185/orient_test.dll')
entry=d.OPTIONAL_HEADER.ImageBase+next(s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'testOrient')
for units in (1,4,32,126):
 for face in (0,8192,65535):
  u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
  for pe in (p,d):u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
  u.mem_map(0x1000000,0x40000)
  def w(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
  mesh,actors,objects,queue,buf,cmd,sp,stop=0x1000100,0x1001000,0x1008000,0x100b000,0x100c000,0x100e000,0x103f000,0x1000000
  w(0x6bee40,3);w(0x531618,mesh);w(mesh+8,7|(6<<16));w(0x532188,actors);w(0x532e6c,objects)
  u.mem_write(0x67ddb0,struct.pack('<HH',units,0));u.mem_write(0x532182,struct.pack('<H',units));u.mem_write(0x532e66,struct.pack('<H',units))
  for i in range(units):
   u.mem_write(actors+i*220,struct.pack('<H',10000+i));u.mem_write(actors+i*220+12,struct.pack('<h',i));u.mem_write(0x67ddb4+i*2,struct.pack('<H',i));w(objects+i*80+8,50000+i*100,60000+i*50)
  selection=bytes(u.mem_read(0x67ddb0,4+units*2));positions=bytes(u.mem_read(objects,units*80))
  w(0x532284,2181);w(0x540a48,queue);w(queue,0,0,0,0,0,0,8192,buf);u.mem_write(buf,b'\xa5'*8192)
  def intercept(c,a,n,data):
   if a not in (0x482b5e,0x4c9530):return
   esp=c.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',c.mem_read(esp,4))[0];c.reg_write(UC_X86_REG_EAX,1);c.reg_write(UC_X86_REG_ESP,esp+4);c.reg_write(UC_X86_REG_EIP,ret)
  u.hook_add(U.UC_HOOK_CODE,intercept)
  w(cmd,0x31444d43,1,22,0x10000|face,0,0,0,0);w(sp,stop,cmd);u.reg_write(UC_X86_REG_ESP,sp)
  u.emu_start(entry,stop,count=2000000)
  assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4 and u.reg_read(UC_X86_REG_EAX)==1
  length=struct.unpack('<I',u.mem_read(queue+8,4))[0];assert length==units*38
  for i in range(units):
   b=bytes(u.mem_read(buf+i*38,38));assert b[:8]==struct.pack('<hbbI',38,3,-1,0)
   assert b[8:]==struct.pack('>4H',16,0,face,0)+struct.pack('>3H4i',1,10000+i,2,50000+i*100,60000+i*50,50000+i*100,60000+i*50)
  assert bytes(u.mem_read(0x67ddb0,4+units*2))==selection and bytes(u.mem_read(objects,units*80))==positions
print('PASS 12 cases: 1/4/32/126 actors, facing, unchanged selection/positions, original per-unit command coordinates. Live turning pending.')
