"""Validate compiled collection reader using read-only cached PC 471b pages."""
import sys,struct,json
from pathlib import Path
sys.path[:0]=['tools/python-deps','tools']
import unicorn as U,pefile
from unicorn.x86_const import *
from probe_engine_state import Memory
m=Memory(int(sys.argv[1]));assert m.profile['id']=='1.8.5-471b'
try:
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);pages=set()
 def fault(uc,access,a,n,v,data):
  page=a&~4095
  if page in pages:return False
  try:b=m.read(page,4096)
  except Exception:return False
  uc.mem_map(page,4096);uc.mem_write(page,b);pages.add(page);return True
 u.hook_add(U.UC_HOOK_MEM_UNMAPPED,fault)
 p=pefile.PE('build/input-185/collection_test.dll');base=p.OPTIONAL_HEADER.ImageBase
 u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image())
 entry=base+next(s.address for s in p.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'collection185_test')
 scratch,sp,stop,out=0x7f000000,0x7f00f000,0x7f00ff00,0x7f001000;u.mem_map(scratch,65536)
 def call(entry,args=(),eax=0,edx=0,esi=0):
  u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),stop,*args))
  for reg,v in [(UC_X86_REG_EAX,eax),(UC_X86_REG_EDX,edx),(UC_X86_REG_ESI,esi),(UC_X86_REG_ESP,sp)]:u.reg_write(reg,v)
  u.emu_start(entry,stop,count=200000)
  assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
  return u.reg_read(UC_X86_REG_EAX)
 # Only emulator scratch writes are allowed (including the original accessor calls).
 writes=[]
 def write(uc,access,a,n,v,data):
  if not scratch<=a<a+n<=scratch+65536:writes.append(hex(a))
 u.hook_add(U.UC_HOOK_MEM_WRITE,write)
 snap=m.snapshot();raw=m.read(snap['objects']['data'],snap['objects']['count']*80)
 cols=sorted({raw[i*80+42] for i in range(snap['objects']['count']) if struct.unpack_from('<H',raw,i*80)[0] and raw[i*80+42]<255})
 rows=[]
 for col in cols:
  assert call(entry,(0,col,0,out))==1;head=bytes(u.mem_read(out,320));bc=struct.unpack_from('<I',head,96)[0];sc=struct.unpack_from('<I',head,128)[0]
  for index in sorted({0,sc//2,sc-1}):
   if index<0:continue
   got=call(entry,(1,col,index,out));expected=call(0x4ce5d7,edx=index,esi=col);assert got==expected
   assert bytes(u.mem_read(out,12))==bytes(u.mem_read(expected,12));rows.append({'collection':col,'kind':'sequence','index':index})
  for index in sorted({0,bc//2,bc-1}):
   if index<0:continue
   assert call(entry,(2,col,index,out))==1;got=bytes(u.mem_read(out,52))
   ref=call(0x4ce3bc,(index,),eax=col);bitmap=struct.unpack('<I',u.mem_read(ref+80,4))[0];expected=bytes(u.mem_read(bitmap,52))
   assert got[:48]==expected[:48] and got[48:52]==expected[44:48]
   w,h,stride=struct.unpack_from('<3H',got);assert 0<w<8192 and 0<h<8192 and stride>=w
   rows.append({'collection':col,'kind':'bitmap','index':index,'width':w,'height':h})
 assert not writes,writes
 Path('research/reports/version-185-collection-verification.json').write_text(json.dumps({'result':'pass','cases':rows,'writes_outside_scratch':writes,'limitations':'Cached read-only live pages; current loaded collections only; palette offset identified statically, pixels not validated here.'},indent=2),encoding='utf8')
 print('PASS:',len(rows),'compiled collection reads match original sequence/bitmap accessors; no emulated game-memory writes')
finally:m.close()
