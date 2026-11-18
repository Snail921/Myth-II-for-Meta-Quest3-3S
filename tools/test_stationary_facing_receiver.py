"""Run original 471b movement receiver: exact-position single point vs duplicate points."""
import struct,sys
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as u
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
pe=pefile.PE('research/extracted/update-1.8.5/Myth II.exe')
for n in (1,2):
 for face in (0,8192,32768,65535):
  c=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
  c.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(0x400000,pe.get_memory_mapped_image());c.mem_map(0x1000000,0x20000)
  actors,objects,points,stack,stop=0x1001000,0x1002000,0x1003000,0x101f000,0x1000000
  def w(a,v):c.mem_write(a,struct.pack('<I',v))
  w(0x532188,actors);w(0x532e6c,objects);w(0x532284,2181);w(0x52f0e0,0x1004000)
  c.mem_write(actors+0x8b,b'\xff');c.mem_write(actors+0x26,struct.pack('<H',1234))
  w(objects+8,50000);w(objects+12,60000)
  position=bytes(c.mem_read(objects+8,12));expected=struct.pack('<ii',50000,60000)*n;c.mem_write(points,expected)
  c.mem_write(stack,struct.pack('<6I',stop,0,n,points,16,face));c.reg_write(UC_X86_REG_ESP,stack)
  c.emu_start(0x47d1cc,stop,count=100000)
  assert c.reg_read(UC_X86_REG_EIP)==stop
  actual=struct.unpack('<H',c.mem_read(actors+0x26,2))[0]
  assert actual==(1234 if n==1 else face),(n,face,actual)
  assert bytes(c.mem_read(objects+8,12))==position
  if n==2:
   assert bytes(c.mem_read(actors+0x22,2))==b'\x02\x00'
   assert bytes(c.mem_read(actors+0x28,16))==expected
   assert struct.unpack('<I',c.mem_read(actors+4,4))[0]&0x600==0
print('PASS original receiver: 1-point regression reproduced; 2 identical points retain 4 headings, unchanged position, no patrol/repeat flags')
