"""Run original OBMO setup through both patched call sites; check camera SNIF isolation."""
import struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/"python-deps"))
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_combat_builders import machine,run,ROOT,STOP,STACK,pefile
from camera_test_support import win32_stubs
c=machine();dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'));base=dll.OPTIONAL_HEADER.ImageBase
c.mem_map(base,(dll.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(base,dll.get_memory_mapped_image());win32_stubs(c,dll)
entry=base+next(e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'MythTestExecute')
mesh=STOP+0xb000
c.mem_write(0x67ec00,struct.pack('<H',3));c.mem_write(0x529bd4,struct.pack('<I',mesh));c.mem_write(mesh,struct.pack('<I4xHH',0x3030746d,3,3));c.mem_write(0x52a9f4,struct.pack('<I',STOP+0xc000));c.mem_write(0x67ec24,struct.pack('<I',100))
def send(op,id=0,flags=0):
 addr=STOP+0x20000;c.mem_write(addr,struct.pack('<8I',0x31444d43,1,op,id,flags,1000,1000,0) if op==12 else struct.pack('<8I',0x31444d43,1,op,id,flags,0,0,0));run(c,entry,[addr],{});return c.reg_read(UC_X86_REG_EAX)
# Guard verifies all signatures before changing any call site.
sites=(0x4a03c7,0x4a042c,0x4321f1);original=[bytes(c.mem_read(x,5)) for x in sites]
c.mem_write(sites[2],b'\x90');assert send(12)==21;assert bytes(c.mem_read(sites[0],5))==original[0];c.mem_write(sites[2],original[2]);assert send(12)==1
# Script ownership prevents listener sync from replacing the scripted camera.
c.mem_write(0x544768,struct.pack('<H',2));c.mem_write(0x657c28,struct.pack("<hh",30,0));prior=bytes(c.mem_read(0x54e43c,40));assert send(12,1234)==1;assert bytes(c.mem_read(0x54e43c,40))==prior;c.mem_write(0x544768,bytes(2))
# Optional script parameters absent. Actual OBMO math and camera readers execute.
c.mem_write(0x49c6a6,b'\x31\xc0\xc3');c.mem_write(0x49c92a,b'\x31\xc0\xc3')
packets=[]
write_addr=STOP+0x28000+3*128
def on_write(c,addr,size,user):
 sp=c.reg_read(UC_X86_REG_ESP);buf,n=struct.unpack('<II',c.mem_read(sp+8,8));packets.append(bytes(c.mem_read(buf,n)))
c.hook_add(UC_HOOK_CODE,on_write,begin=write_addr,end=write_addr)
for site in sites[:2]:
 dest=STOP+0x25000;c.mem_write(dest,struct.pack('<ii',12000,18000))
 c.mem_write(STACK,struct.pack('<II',STOP+0x24000,16384));c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_EAX,dest)
 for r,v in [(UC_X86_REG_EBX,111),(UC_X86_REG_ESI,222),(UC_X86_REG_EDI,333),(UC_X86_REG_EBP,444)]:c.reg_write(r,v)
 c.emu_start(site,site+5,count=100000)
 assert c.reg_read(UC_X86_REG_EIP)==site+5 and c.reg_read(UC_X86_REG_ESP)==STACK
 assert [c.reg_read(r) for r in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)]==[111,222,333,444]
 packet=struct.unpack('<9I',packets[-1]);assert packet[0]==0x314d4143 and packet[2:8]==(0x3030746d,100,12000,18000,16384,mesh) and packet[1]==packet[8]
 assert struct.unpack('<ii',c.mem_read(0x657c14,8))==(12000,18000)
assert len(packets)==2 and struct.unpack('<I',packets[1][4:8])[0]==2
# Original camera SNIF is bypassed only for VR tutorial camera lessons, not ordinary lessons.
c.mem_write(0x46a80a,b'\xb8\x78\x56\x34\x12\xc3')
def legacy(case,tag=0x3030746d,tick=100):
 c.mem_write(mesh,struct.pack('<I',tag));c.mem_write(0x561872,struct.pack('<H',case));c.mem_write(0x67ec24,struct.pack('<I',tick));c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_EAX,99);c.emu_start(0x4321f1,0x4321f6,count=10000);return c.reg_read(UC_X86_REG_EAX)
assert legacy(51)==99 and legacy(55)==99
assert legacy(100)==0x12345678 and legacy(51,tag=1)==0x12345678 and legacy(51,tick=161)==0x12345678
# Legacy camera movement must not cancel an in-flight scenario move, even before VR receives it.
c.mem_write(0x450f61,b'\xb8\x78\x56\x34\x12\xc3')
c.mem_write(0x67ec24,struct.pack('<I',100));c.mem_write(mesh,struct.pack('<I',0x3030746d))
def camera_legacy():
 c.reg_write(UC_X86_REG_ESP,STACK);c.reg_write(UC_X86_REG_EAX,99);c.emu_start(0x4321f6,0x4321fb,count=10000);return c.reg_read(UC_X86_REG_EAX)
c.mem_write(0x544768,struct.pack('<H',2));c.mem_write(0x657c28,struct.pack('<hh',30,1));assert camera_legacy()==99
c.mem_write(0x657c28,struct.pack('<hh',30,31));assert send(12,flags=1)==1;assert camera_legacy()==99
# Mode bit can remain set after interpolation finishes. VR's grace-period flag owns the remaining lock.
assert send(12,flags=0)==1;assert camera_legacy()==0x12345678
print('PASS both original OBMO setup calls, event payloads, registers/stack, script ownership, signature guard, tutorial-only legacy isolation and expiry')
