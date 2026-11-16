import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps');import pefile,unicorn as U
from unicorn.x86_const import *
DLL=pefile.PE('build/integration-185/full-adapters-test.dll')
def run(new,n,kind):
 p=pefile.PE('research/extracted/update-'+('1.8.5' if new else '1.8.1')+'/Myth II.exe');u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
 for pe in ([p,DLL] if new else [p]):u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
 u.mem_map(0x1000000,0x40000)
 def w(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 actors,slots,opts,queue,buf,sp,stop=0x1001000,0x1009000,0x1009100,0x1009200,0x100a000,0x103f000,0x1000000
 w(0x532188 if new else 0x529f54,actors)
 for i in range(n):u.mem_write(actors+i*(220 if new else 216),struct.pack('<H',10000+i));u.mem_write(slots+i*2,struct.pack('<H',i))
 w(0x532284 if new else 0x52a744,2181);w(0x540a48 if new else 0x52f4e4,queue);w(queue,*([0]*(6 if new else 5)),4096,buf);u.mem_write(buf,b'\xa5'*4096)
 if kind=='ground_selected':u.mem_write(opts,struct.pack('<HHiii',0,0,91000,78000,300))
 else:u.mem_write(opts,struct.pack('<4H',0,0,115,12345))
 if new:
  u.mem_write(0x67ddb0,struct.pack('<HH',n,0));u.mem_write(0x67ddb4,bytes(u.mem_read(slots,n*2)));w(sp,stop,opts)
  entry=DLL.OPTIONAL_HEADER.ImageBase+next(s.address for s in DLL.DIRECTORY_ENTRY_EXPORT.symbols if s.name.decode()==kind)
 else:entry=0x446948 if kind=='ground_selected' else 0x446a17;w(sp,stop,0xffffffff,opts);u.reg_write(UC_X86_REG_ECX,slots);u.reg_write(UC_X86_REG_EDX,n)
 u.reg_write(UC_X86_REG_ESP,sp);saved={UC_X86_REG_EBX:123,UC_X86_REG_ESI:456,UC_X86_REG_EDI:789,UC_X86_REG_EBP:321}
 for r,v in saved.items():u.reg_write(r,v)
 u.emu_start(entry,stop,count=300000);assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
 for r,v in saved.items():assert u.reg_read(r)==v
 length=struct.unpack('<I',u.mem_read(queue+8,4))[0];assert 8<length<4096;assert bytes(u.mem_read(buf+length,16))==b'\xa5'*16
 return bytes(u.mem_read(buf,length)).hex()
rows=[]
for n in [1,4,32,126]:
 for kind in ['ground_selected','pickup_selected']:
  a,b=run(False,n,kind),run(True,n,kind);assert a==b,(kind,n,a,b);rows.append({'units':n,'kind':kind,'bytes':len(a)//2})
Path('research/reports/version-185-ground-pickup-test.json').write_text(json.dumps({'result':'pass','cases':rows,'scope':'Compiled 471b adapters versus original 434 command serialization; no live gameplay'},indent=2))
print('PASS',len(rows),'ground attack/pickup adapter payloads match original 434 engine')

