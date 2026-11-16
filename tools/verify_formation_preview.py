"""Real x86 formation calculator vs compiled thunk; synthetic initialized map/player data."""
import sys,struct,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile,unicorn as u
from unicorn.x86_const import *
exe=pefile.PE(str(ROOT/'research/extracted/update-1.8.1/Myth II.exe'))
dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'));base=dll.OPTIONAL_HEADER.ImageBase
entry=base+next(x.address for x in dll.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'MythTestFormation')
def run(form,units,count,player,thunk,facing=None):
 c=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
 for pe,b in [(exe,0x400000),(dll,base)]:c.mem_map(b,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(b,pe.get_memory_mapped_image())
 c.mem_map(0x1000000,0x40000)
 def w(a,*v):c.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 for b,fn in [(0x676140,math.cos),(0x675940,math.sin)]:c.mem_write(b,struct.pack('<1024h',*[round(fn(i*math.tau/1024)*16384) for i in range(1024)]))
 c.mem_write(0x67f2bc,struct.pack('<f',math.tau));w(0x529f54,0x1001000);w(0x52a9f4,0x1009000);w(0x52a744,2181);c.mem_write(0x67ec62,struct.pack('<HH',191,223))
 # A custom per-player formation tag exercises the actual runtime path, not only defaults.
 w(0x545030,0x1014000);tag=bytearray(exe.get_data(0x127b20,40));struct.pack_into('<H',tag,2,1024);c.mem_write(0x1014004,bytes(tag))
 for i in range(units):c.mem_write(0x1001000+i*216+12,struct.pack('<h',i));w(0x1009000+i*80+8,95000+(i%10)*500,78000+(i//10)*300)
 c.mem_write(0x1010000,struct.pack('<'+'H'*units,*range(units)))
 for j in range(count):w(0x1011000+j*8,95000+j*500,79000-j*1000)
 c.mem_write(0x1012000,struct.pack('<4H',(1 if form<0 else 8)|(16 if facing is not None else 0),form&65535,facing or 0,0))
 args=[0x1012000,player&0xffffffff,0x1010000,units,0x1011000,0x1015000,0x1017000]
 w(0x103f000,0x1000000,*args,*([count] if thunk else []));c.reg_write(UC_X86_REG_ESP,0x103f000);c.reg_write(UC_X86_REG_EAX,count)
 illegal=[]
 def check(cpu,access,addr,size,value,data):
  if not (0x1030000<=addr<0x1040000 or 0x1015000<=addr<0x1015fc0 or 0x1017000<=addr<0x10170fc):illegal.append(hex(addr))
 c.hook_add(u.UC_HOOK_MEM_WRITE,check)
 c.emu_start(entry if thunk else 0x4481d7,0x1000000,count=2000000)
 assert c.reg_read(UC_X86_REG_EIP)==0x1000000
 assert not illegal,(form,units,count,illegal[:5])
 xy=bytes(c.mem_read(0x1015000,4032));faces=bytes(c.mem_read(0x1017000,252))
 for j in range(count):
  for i in range(units):
   x,y=struct.unpack_from('<ii',xy,(j*126+i)*8);assert 0<=x<223*512 and 0<=y<191*512
 return xy,faces
cases=0
for form in range(-1,10):
 for units in [1,4,126]:
  for count in [1,4]:
   for player in [-1,0]:
    assert run(form,units,count,player,False)==run(form,units,count,player,True)
    cases+=1
for form in range(-1,10):
 for facing in [0,16384,32768,49152]:
  for count in [1,4]:
   assert run(form,4,count,0,False,facing)==run(form,4,count,0,True,facing)
   cases+=1
print('PASS',cases,'formation cases: original/thunk byte equality, no writes outside outputs/stack; units 1/4/126, points 1/4, defaults/player tag')
(ROOT/'research/reports/formation-preview-verification.json').write_text(json.dumps(dict(cases=cases,result='pass',state='synthetic initialized trig/player/map tables',original_calculator='0x4481d7',outside_output_writes=0),indent=2))
