"""Compiled preview dispatcher, original calculator and file reply; only OS I/O/eligibility stubbed."""
from verify_group_selection import *
import math
from verify_combat_builders import QUEUE
for addr,fn in [(0x676140,math.cos),(0x675940,math.sin)]:c.mem_write(addr,struct.pack('<1024h',*[round(fn(i*math.tau/1024)*16384) for i in range(1024)]))
c.mem_write(0x67f2bc,struct.pack('<f',math.tau));c.mem_write(0x67ec62,struct.pack('<HH',191,223));c.mem_write(0x52a744,struct.pack('<I',2181));c.mem_write(0x52ae68,struct.pack('<i',-1))
c.mem_write(0x553fd8,struct.pack('<HH',4,0));c.mem_write(0x553fdc,struct.pack('<4H',0,1,2,3))
for i in range(4):
 c.mem_write(ACTORS+i*216+12,struct.pack('<h',i));c.mem_write(0x100c000+i*80+8,struct.pack('<ii',95000+i*500,78000+i*300))
callbacks={};packets=[]
for block in dll.DIRECTORY_ENTRY_IMPORT:
 for imp in block.imports:
  if imp.name in (b'CreateFileA',b'WriteFile',b'CloseHandle'):
   addr=0x1028000+len(callbacks)*16;callbacks[addr]=imp.name;c.mem_write(imp.address,struct.pack('<I',addr))
def io(cpu,address,size,user):
 esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
 if address==0x434b6d:
  cpu.reg_write(UC_X86_REG_EAX,0x1029000);cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret);return
 if address==0x468e13:cpu.reg_write(UC_X86_REG_EAX,4);cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret);return
 name=callbacks.get(address)
 if name is None:return
 pop={b'CreateFileA':28,b'WriteFile':20,b'CloseHandle':4}[name]
 if name==b'WriteFile':
  handle,ptr,n,written,_=struct.unpack('<5I',cpu.mem_read(esp+4,20));packets.append(bytes(cpu.mem_read(ptr,n)));cpu.mem_write(written,struct.pack('<I',n))
 cpu.reg_write(UC_X86_REG_EAX,1);cpu.reg_write(UC_X86_REG_ESP,esp+4+pop);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,io)
# Stub only cell lookup; execute original triangle classification and prohibition mask.
for kind in range(16):
 c.mem_write(0x1029004,bytes([kind|(kind<<4)]))
 for i,mask in enumerate([65535,0,1<<kind,1<<((kind+1)%16)]):c.mem_write(ACTORS+i*216+0x1a,struct.pack('<H',mask))
 assert send(15,struct.pack('<5I',0,0,95000,79000,0))==1
 assert packets[-1][4300:4304]==bytes([1,0,1,0]),kind
print('PASS all 16 terrain kinds with allowed/blocked/mixed unit masks')
before=bytes(c.mem_read(ACTORS,127*216));queue=bytes(c.mem_read(QUEUE,28));voices=len(voice)
assert send(15,struct.pack('<5I',0,0,95000,79000,0))==1
p=packets[-1];assert len(p)==4808 and struct.unpack_from('<I',p,0)[0]==0x32565046
assert struct.unpack_from('<II',p,8)==(4,1) and p[4:8]==p[-4:]
assert bytes(c.mem_read(ACTORS,127*216))==before and bytes(c.mem_read(QUEUE,28))==queue and len(voice)==voices
for i in range(3):assert send(13,struct.pack('<5I',0,int(i==0),95000+i*100,79000,0))==1
assert send(15,struct.pack('<5I',0,1,97000,78000,0))==1
assert struct.unpack_from('<II',packets[-1],8)==(4,4)
assert bytes(c.mem_read(QUEUE,28))==queue
assert send(13,struct.pack('<5I',0,0,97000,78000,0))==1
assert send(15,struct.pack('<5I',0,1,98000,78000,0))==16
print('PASS preview reply, 4-point staged preview, no actor/queue/voice changes, fifth point rejected')

assert send(15,struct.pack('<5I',0,0,95000,79000,0x10000))==1
east=packets[-1]
assert send(15,struct.pack('<5I',0,0,95000,79000,0x14000))==1
north=packets[-1]
assert east[268:4300]!=north[268:4300], 'explicit facing must rotate original preview slots'
assert bytes(c.mem_read(QUEUE,28))==queue
print('PASS explicit facing rotates preview without movement enqueue')
