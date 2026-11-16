import sys,struct,json
from pathlib import Path
sys.path[:0]=['tools/python-deps','tools']
import unicorn as U,pefile
from unicorn.x86_const import *
from probe_engine_state import Memory
m=Memory(int(sys.argv[1]));u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);pages={}
def fault(c,access,a,n,v,data):
 page=a&~4095
 if page in pages:return False
 try:b=m.read(page,4096)
 except Exception as e:print('Unreadable',hex(a),e);return False
 c.mem_map(page,4096,U.UC_PROT_READ|U.UC_PROT_EXEC);c.mem_write(page,b);pages[page]=b;return True
u.hook_add(U.UC_HOOK_MEM_UNMAPPED,fault)
p=pefile.PE('build/integration-185/collectors_test.dll');base=p.OPTIONAL_HEADER.ImageBase
u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image());entry=base+next(s.address for s in p.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'collector185_test')
scratch,sp,stop,out=0x78000000,0x780ff000,0x780fff00,0x78100000;u.mem_map(scratch,4*1024*1024)
rows=[]
try:
 for mode in range(3):
  u.mem_write(sp,struct.pack('<4I',stop,mode,out,0));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(entry,stop,count=200000000)
  assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
  n=u.reg_read(UC_X86_REG_EAX)
  if mode==0:
   assert 4<=n<=262144;raw=bytes(u.mem_read(out,n));count=struct.unpack_from('<I',raw)[0];pos=4
   for i in range(count):w,h=struct.unpack_from('<2I',raw,pos+32);assert 0<w<=256 and 0<h<=256;pos+=64+w*h
   assert pos==n
  elif mode==1:
   assert n<=2048;raw=bytes(u.mem_read(out,n*64));kinds={}
   for i in range(n):kind=struct.unpack_from('<I',raw,i*64+60)[0];kinds[kind]=kinds.get(kind,0)+1
   print('FX kinds',kinds)
  else:assert n==4;raw=bytes(u.mem_read(out,n*131076))
  Path(f'build/integration-185/collector-{mode}.bin').write_bytes(raw);rows.append({'mode':mode,'result':n});print('PASS mode',mode,'result',n)
 # Retain the exact lazily-fetched source pages for an ARM64 contract test.
 with Path('build/integration-185/full-memory.fixture').open('wb') as f:
  f.write(struct.pack('<I',len(pages)))
  for a,b in sorted(pages.items()):f.write(struct.pack('<I',a)+b)
 Path('research/reports/version-185-collectors-test.json').write_text(json.dumps({'result':'pass','rows':rows,'pages':len(pages),'scope':'Production headers in x86 emulator; original memory read-only; non-atomic cached pages'},indent=2))
finally:m.close()
