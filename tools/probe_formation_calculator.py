import sys,struct,math
sys.path.insert(0,'tools/python-deps');import pefile,unicorn as u
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.1/Myth II.exe')
for form in range(10):
 c=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32);c.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(0x400000,p.get_memory_mapped_image());c.mem_map(0x1000000,0x40000)
 def w(a,*v):c.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 for base,fn in [(0x676140,math.cos),(0x675940,math.sin)]:c.mem_write(base,struct.pack('<1024h',*[round(fn(i*math.tau/1024)*16384) for i in range(1024)]))
 c.mem_write(0x67f2bc,struct.pack('<f',6.283185307))
 w(0x529f54,0x1001000);w(0x52a9f4,0x1003000);w(0x52a744,2181);c.mem_write(0x67ec62,struct.pack('<HH',191,223))
 for i in range(4):c.mem_write(0x1001000+i*216+12,struct.pack('<h',i));w(0x1003000+i*80+8,95000+i*500,78000+i*300)
 c.mem_write(0x1005000,struct.pack('<4H',0,1,2,3));w(0x1006000,95000,79000);c.mem_write(0x1007000,struct.pack('<4H',8,form,0,0))
 w(0x103f000,0x1000000,0x1007000,0xffffffff,0x1005000,4,0x1006000,0x1008000,0x100a000)
 c.reg_write(UC_X86_REG_ESP,0x103f000);c.reg_write(UC_X86_REG_EAX,1)
 writes=set()
 def trace(c,a,addr,size,value,data):
  if addr<0x1000000:writes.add(addr)
 c.hook_add(u.UC_HOOK_MEM_WRITE,trace)
 try:c.emu_start(0x4481d7,0x1000000,count=1000000);print(form,hex(c.reg_read(UC_X86_REG_EIP)),[hex(a) for a in sorted(writes)])
 except Exception as e:print(form,e,hex(c.reg_read(UC_X86_REG_EIP)))
