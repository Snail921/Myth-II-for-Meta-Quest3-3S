"""Compiled op18 + call-site patch + original distance, with only Win32 APIs stubbed."""
import struct, math
from verify_combat_builders import machine, run, ROOT, STOP, STACK, pefile
from unicorn.x86_const import *

c=machine()
dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'));base=dll.OPTIONAL_HEADER.ImageBase
c.mem_map(base,(dll.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(base,dll.get_memory_mapped_image())
entry=base+next(e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'MythTestExecute')
# VirtualProtect writes the previous protection and succeeds; cache flush succeeds.
stubs={b'VirtualProtect':bytes.fromhex('8b442410c70020000000b801000000c21000'),b'FlushInstructionCache':bytes.fromhex('b801000000c20c00')}
for desc in dll.DIRECTORY_ENTRY_IMPORT:
 for imp in desc.imports:
  if imp.name in stubs:
   addr=STOP+0x28000+list(stubs).index(imp.name)*128;c.mem_write(addr,stubs[imp.name]);c.mem_write(imp.address,struct.pack('<I',addr))
def send(percent,flags=0):
 c.mem_write(STOP+0x20000,struct.pack('<8I',0x31444d43,1,18,percent,flags,0,0,0));run(c,entry,[STOP+0x20000],{});return c.reg_read(UC_X86_REG_EAX)
original=bytes(c.mem_read(0x4cc603,5))
assert send(99)==16 and send(201)==16 and send(150,1)==16
c.mem_write(0x4cc603,b'\x90');assert send(150)==21;c.mem_write(0x4cc603,original)
assert send(100)==1 and bytes(c.mem_read(0x4cc603,5))==original
for percent in (150,200,100):
 assert send(percent)==1
 for xy in [(15360,0),(30000,30000),(100000,0),(512,768),(-15360,0),(2147483647,-2147483648)]:
  a=STOP+0x22000;b=a+16;c.mem_write(a,struct.pack('<ii',*xy));c.mem_write(b,bytes(8))
  c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_EAX,a);c.reg_write(UC_X86_REG_EDX,b)
  c.reg_write(UC_X86_REG_EBX,123);c.reg_write(UC_X86_REG_ESI,456);c.reg_write(UC_X86_REG_EDI,789)
  c.emu_start(0x4cc603,0x4cc608,count=10000);result=c.reg_read(UC_X86_REG_EAX)
  assert c.reg_read(UC_X86_REG_EIP)==0x4cc608 and c.reg_read(UC_X86_REG_ESP)==STACK
  assert [c.reg_read(r) for r in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI)]==[123,456,789]
  scaled=tuple(int(v*100/percent) for v in xy)
  run(c,0x46bfa2,[],{UC_X86_REG_EAX:a,UC_X86_REG_EDX:b})
  if percent==100:assert result==c.reg_read(UC_X86_REG_EAX)
  elif max(map(abs,scaled))>=32767:assert result==32767
  else:
   c.mem_write(a,struct.pack('<ii',*scaled));run(c,0x46bfa2,[],{UC_X86_REG_EAX:a,UC_X86_REG_EDX:b});assert result==c.reg_read(UC_X86_REG_EAX)
 print('PASS actual patched distance call',percent)
assert bytes(c.mem_read(0x46bfa2,8))==bytes(machine().mem_read(0x46bfa2,8))
print('PASS signature guard, bounds, register preservation, 1x restoration and unchanged shared distance function')
