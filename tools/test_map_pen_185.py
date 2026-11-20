"""Emulate compiled command adapter plus actual 471b annotation routines; no live writes."""
from pathlib import Path
import sys,subprocess,struct,json
sys.path.insert(0,str(Path(__file__).parent/'python-deps'))
import pefile,unicorn as U
from unicorn.x86_const import *
w=Path('build/map-pen-test');w.mkdir(exist_ok=True)
w.joinpath('test.c').write_text('#include "win_min.h"\nint _fltused=0;struct Command {U magic,seq,op,id,flags;I x,y;U reserved;};\n#include "map_pen.h"\nU test(const struct Command*c){return map_pen_command(c);}',encoding='utf-8')
a=Path('src/input/engine185/game_calls.S').read_text();w.joinpath('test.S').write_text('.text\n'+a[a.index('.globl _map_pen_mode185'):],encoding='utf-8')
bin=Path('tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin')
subprocess.run([str(bin/'clang.exe'),'--target=i686-pc-windows-msvc','-O2','-ffreestanding','-fno-stack-protector','-I','src/input','-I','src/input/engine185','-c',str(w/'test.c'),'-o',str(w/'test.obj')],check=True)
subprocess.run([str(bin/'clang.exe'),'--target=i686-pc-windows-msvc','-c',str(w/'test.S'),'-o',str(w/'calls.obj')],check=True)
subprocess.run([str(bin/'ld.lld.exe'),'-flavor','link','/dll','/noentry','/machine:x86','/nodefaultlib','/safeseh:no','/export:test','/out:'+str(w/'test.dll'),str(w/'test.obj'),str(w/'calls.obj')],check=True)
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE(str(w/'test.dll'));u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
for pe in (p,d):u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
u.mem_map(0x1000000,65536)
def put(a,v):u.mem_write(a,struct.pack('<I',v))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
put(0x6bee46,1);put(0x5332dc,1);put(0x5332e0,0);u.mem_write(0x533296,b'\x02\x00');put(0x53329c,0x1001000);u.mem_write(0x1001072,b'\x00');put(0x66b9ca,0);put(0x66b9a0,0)
entry=d.OPTIONAL_HEADER.ImageBase+next(s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'test');packets=[]
def hook(c,a,n,data):
 if a not in (0x49fa74,0x47b9b5):return
 sp=c.reg_read(UC_X86_REG_ESP)
 if a==0x47b9b5:
  flag,count,points=struct.unpack('<III',c.mem_read(sp+4,12));packets.append((get(flag)&65535,list(c.mem_read(points,count*2))))
 ret=get(sp);c.reg_write(UC_X86_REG_ESP,sp+4);c.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(U.UC_HOOK_CODE,hook)
def call(action,x=0,y=0):
 sp=0x100e000;stop=0x1000000;addr=0x1002000;u.mem_write(addr,struct.pack('<8I',0,1,25,action,0,x,y,0));u.mem_write(sp,struct.pack('<II',stop,addr))
 u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ESI,0x12345678);u.emu_start(entry,stop,count=100000)
 assert u.reg_read(UC_X86_REG_ESP)==sp+4 and u.reg_read(UC_X86_REG_ESI)==0x12345678
 return u.reg_read(UC_X86_REG_EAX)
assert call(1)==1 and get(0x66b990)==1 and packets[-1][0]==1
assert call(2,0,9)==16
for xy in [(10,20),(15,25),(19,27),(24,30),(28,35),(30,40),(35,40),(40,40)]:
 assert call(2,*xy)==1
 # A normal engine mouse-up tick runs between VR requests. No pending data
 # must remain for that tick to turn into a single-point stroke.
 assert (get(0x66b9a0)&65535)==0
 before=bytes(u.mem_read(0x66b9ca,402))
 sp=0x100e000;u.mem_write(sp,struct.pack('<II',0x1000000,1));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ESI,0)
 u.emu_start(0x49f9e1,0x1000000,count=100000)
 assert before==bytes(u.mem_read(0x66b9ca,402))
assert call(3)==1 and (get(0x66b9a0)&65535)==0
n=get(0x66b9ca)&65535;points=list(u.mem_read(0x66b9cc,n*2));assert points[:2]==[0,2] and points[-2:]==[0,0]
payload=[v for flags,ps in packets if flags==0 for v in ps]
assert payload[4:]==points[4:], (payload,points)
assert payload[4:].count(0)==2, payload # only final stroke delimiter
assert len([ps for flags,ps in packets if flags==0 and any(ps)])==2 # two live batches, one end
assert [40,40,0,0]==payload[-4:]
# Keep another team's stroke; original eraser must remove only local team's group.
u.mem_write(0x66b9cc+n*2,bytes([0,3,60,60,70,70,0,0]));u.mem_write(0x66b9ca,struct.pack('<H',n+4))
assert call(4)==1 and packets[-1][0]==2
assert list(u.mem_read(0x66b9cc,8))==[0,3,60,60,70,70,0,0]
# Long straight strokes are reduced; duplicate heartbeats add no entries.
packets.clear()
for x in range(10,90):assert call(2,x,50)==1
for i in range(12):assert call(2,89,50)==1
assert call(3)==1
payload=[v for flags,ps in packets if flags==0 and (any(ps) or ps==[0,0]) for v in ps]
assert payload[:2]==[10,50] and payload[-4:]==[89,50,0,0]
assert len(payload)//2==22, len(payload)//2
# A new stroke starts separately, with corners and reversal retained.
packets.clear()
for xy in [(20,20),(30,20),(30,30),(20,30)]:assert call(2,*xy)==1
assert call(3)==1
assert [v for flags,ps in packets if flags==0 for v in ps]==[20,20,30,20,30,30,20,30,0,0]
# Overflow the original 200-entry store while preserving its bounds.
for i in range(400):
 assert call(2,20+i%150,20+(i%2)*100)==1
 assert (get(0x66b9ca)&65535)<=200
assert call(3)==1 and (get(0x66b9ca)&65535)<=200
assert call(0)==1 and get(0x66b990)==0
put(0x6bee46,2);assert call(1)==20
Path('build/map-pen-test/result.json').write_text(json.dumps({'pass':True,'packets':packets,'scope':'Compiled adapter + original local append/erase; network transport and UI invalidation intercepted'},indent=2))
print('PASS compiled ABI, original team erasure, point continuity, end markers, flags, invalid inputs and replay rejection')
