"""Compare portable decoder with original x86 code in an isolated emulator."""
import sys,struct,subprocess,json,hashlib
from pathlib import Path
sys.path[:0]=['tools/python-deps','research/references/mythextract/scripts']
import unicorn as U,pefile,myth_headers
from unicorn.x86_const import *
work=Path('build/detail-test');work.mkdir(exist_ok=True)
plugin=Path('Myth II - Plugins/Magma - Detail Texture Mega Pack/Magma - Detail Texture Megapack')
data=plugin.read_bytes();tags=myth_headers.get_mono_tags(data,myth_headers.parse_mono_header(plugin.name,data))
samples=[];seen=set()
for t in tags:
 if str(t.tag_type)!='dtex':continue
 b=data[t.tag_data_offset:t.tag_data_offset+t.tag_data_size];head=struct.unpack_from('<I',b,20)[0];mode=head>>6&3;w=head>>20;h=head>>8&4095
 key=(mode,w==h)
 if key in seen or w>512:continue
 seen.add(key);path=work/f'{len(samples)}.dtex';path.write_bytes(b);samples.append((path,b,mode,w,h))
source='''package org.mythvr.standalone;
import java.nio.file.*;
public class DetailTest {public static void main(String[] args)throws Exception{
 for(String arg:args){byte[] raw=Files.readAllBytes(Paths.get(arg));DetailCodec.Image im=DetailCodec.decode(raw);Files.write(Paths.get(arg+".rgb"),im.rgb);System.out.println(arg+" "+im.width+"x"+im.height);}
}}
'''
(work/'DetailTest.java').write_text(source)
jdk=Path('tools/runtime/jdk-21.0.13+9/bin')
subprocess.run([str(jdk/'javac.exe'),'-d',str(work),'src/standalone/DetailCodec.java',str(work/'DetailTest.java')],check=True)
subprocess.run([str(jdk/'java.exe'),'-cp',str(work),'org.mythvr.standalone.DetailTest',*[str(s[0]) for s in samples]],check=True)
p=pefile.PE('research/extracted/update-1.8.1/Myth II.exe');base=p.OPTIONAL_HEADER.ImageBase
results=[]
for path,b,mode,w,h in samples:
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image())
 scratch=0x10000000;u.mem_map(scratch,64*1024*1024);sp=scratch+0x10000;stop=scratch+0xff00;out=scratch+0x20000;src=scratch+0x30000;heap=[scratch+0x200000]
 u.mem_write(src,b[20:]);u.mem_write(sp,struct.pack('<4I',stop,out,out+4,out+8));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EAX,src)
 def ret(value,pop=0):
  esp=u.reg_read(UC_X86_REG_ESP);addr=struct.unpack('<I',u.mem_read(esp,4))[0];u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_ESP,esp+4+pop);u.reg_write(UC_X86_REG_EIP,addr)
 def allocate(size):
  ptr=heap[0];heap[0]+=(size+15)&~15;assert heap[0]<scratch+64*1024*1024;return ptr
 def new(uc,a,n,d):ret(allocate(struct.unpack('<I',u.mem_read(u.reg_read(UC_X86_REG_ESP)+4,4))[0]))
 def free(uc,a,n,d):ret(0)
 u.hook_add(U.UC_HOOK_CODE,new,begin=0x4f3088,end=0x4f3088)
 # IAT thunks are replaced only inside the emulator.
 for entry in p.DIRECTORY_ENTRY_IMPORT:
  for imp in entry.imports:
   if imp.name in [b'HeapAlloc',b'HeapFree']:
    addr=scratch+(0x100 if imp.name==b'HeapAlloc' else 0x200);u.mem_write(imp.address,struct.pack('<I',addr))
    if imp.name==b'HeapAlloc':
     def alloc(uc,a,n,d):ret(allocate(struct.unpack('<I',u.mem_read(u.reg_read(UC_X86_REG_ESP)+12,4))[0]),12)
     u.hook_add(U.UC_HOOK_CODE,alloc,begin=addr,end=addr)
    else:
     def hf(uc,a,n,d):ret(1,12)
     u.hook_add(U.UC_HOOK_CODE,hf,begin=addr,end=addr)
 # Locate the final CRT free call from the decoder's bytes.
 import capstone
 md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
 calls=[int(i.op_str,16) for i in md.disasm(p.get_data(0xb7f20,0x4e0),0x4b7f20) if i.mnemonic=='call' and i.op_str.startswith('0x')]
 u.hook_add(U.UC_HOOK_CODE,free,begin=calls[-1],end=calls[-1])
 u.emu_start(0x4b7f20,stop,count=200000000)
 assert u.reg_read(UC_X86_REG_EIP)==stop,hex(u.reg_read(UC_X86_REG_EIP))
 ptr,rw,rh=struct.unpack('<3I',u.mem_read(out,12));assert (rw,rh)==(w,h)
 expected=bytes(u.mem_read(ptr,w*h*3));got=Path(str(path)+'.rgb').read_bytes()
 differences=[i for i,(x,y) in enumerate(zip(expected,got)) if x!=y]
 (work/(path.stem+'.original.rgb')).write_bytes(expected)
 assert not differences,(path,mode,len(differences),differences[:8],expected[:30].hex(),got[:30].hex())
 results.append(dict(sample=str(path),mode=mode,width=w,height=h,sha256=hashlib.sha256(got).hexdigest()));print('PASS',path,mode,w,h,flush=True)
Path('research/reports/detail-decoder-verification.json').write_text(json.dumps({'result':'pass','samples':results,'reference':'Original 1.8.1 decoder 0x4b7f20, isolated Unicorn memory; no live game changes'},indent=2))
