import sys,struct,random,subprocess
from pathlib import Path
sys.path.insert(0,'tools/python-deps');import pefile,unicorn as U
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(0x400000,p.get_memory_mapped_image());base=0x10000000;u.mem_map(base,65536);stop=base+0x100;sp=base+0x8000;control=base+0x1000;rng=random.Random(185);source=bytearray(b'HUECASE1');expected=bytearray()
for i in range(2048):
 pixel=rng.randrange(1<<24);c=struct.pack('<4B2H',rng.choice([0,1,3]),rng.choice([0,255]),rng.randrange(256),rng.randrange(256),rng.randrange(65536),rng.randrange(65536));u.mem_write(control,c);u.mem_write(sp,struct.pack('<II',stop,pixel));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EAX,control);u.emu_start(0x44ff10,stop,count=1000);expected+=struct.pack('<I',u.reg_read(UC_X86_REG_EAX));source+=struct.pack('<I',pixel)+c
root=Path('build/hd-test');(root/'hues.bin').write_bytes(source);adb='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
def call(*a):subprocess.run([adb,*a],check=True,stdout=subprocess.DEVNULL)
call('push',str(root/'hues.bin'),'/data/local/tmp/hd-hues.bin');call('shell','/data/local/tmp/myth-hd-test /data/local/tmp/hd-hues.bin /data/local/tmp/hd-hues.out');call('pull','/data/local/tmp/hd-hues.out',str(root/'hues.out'));actual=(root/'hues.out').read_bytes();assert actual==expected, next((i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b);print('PASS 2048 hue conversions byte-exact against original 1.8.5')

# Compare actual packed images with the original 0x4500a0 decoder (neutral hues).
samples=[]
for plugin in Path('Myth II - Plugins').rglob('Magma - HD Megapack *'):
 with plugin.open('rb') as f:
  h=f.read(128);entries,count=struct.unpack_from('>HH',h,100);f.seek(128+entries*112);tags=[f.read(64) for _ in range(count)]
  for t in tags:
   if t[36:40]!=b'd256':continue
   off=struct.unpack_from('>I',t,44)[0];f.seek(off);h=f.read(64);n,r=struct.unpack_from('>II',h,8);f.seek(off+64+r);ref=f.read(128);bo,sz=struct.unpack_from('>II',ref,64)
   if sz>200000:continue
   f.seek(off+64+bo);data=f.read(sz);w,hh,_,flags=struct.unpack_from('>4H',data);enc=struct.unpack_from('>H',data,16)[0]
   if enc==1 and w*hh<60000:samples.append(data)
   if len(samples)>=8:break
 if len(samples)>=8:break
u.mem_map(base+65536,16*1024*1024);bitmap=base+0x2000;table=base+0x3000;pixels=base+0x10000;output=base+0x100000
# memset interception: CRT state is deliberately not initialized in this isolated emulator.
def hook(uc,address,size,user):
 if address==0x4f6f10:
  sp=uc.reg_read(UC_X86_REG_ESP);ret,dest,val,n=struct.unpack('<4I',uc.mem_read(sp,16));uc.mem_write(dest,bytes([val&255])*n);uc.reg_write(UC_X86_REG_EAX,dest);uc.reg_write(UC_X86_REG_ESP,sp+4);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(U.UC_HOOK_CODE,hook)
for idx,data in enumerate(samples):
 w,h,_,flags=struct.unpack_from('>4H',data);skip=52+(0 if flags&32 else 4*(h-1));header=bytearray(52);struct.pack_into('<2h',header,0,w,h);struct.pack_into('<I',header,44,pixels);u.mem_write(bitmap,bytes(header));u.mem_write(table,bytes(156));u.mem_write(pixels,data[skip:]);u.mem_write(sp,struct.pack('<3I',stop,bitmap,output));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EDX,table);u.emu_start(0x4500a0,stop,count=10000000);native=bytes(u.mem_read(output,w*h*4))
 path=root/f'image{idx}.bin';path.write_bytes(data);call('push',str(path),'/data/local/tmp/hd-image.bin');call('shell','/data/local/tmp/myth-hd-test /data/local/tmp/hd-image.bin /data/local/tmp/hd-image.out');call('pull','/data/local/tmp/hd-image.out',str(root/'image.out'));actual=(root/'image.out').read_bytes();converted=bytearray()
 for j in range(0,len(actual),4):
  r,g,b,a=actual[j:j+4];converted+=bytes((b*(a+1)>>8,g*(a+1)>>8,r*(a+1)>>8,a))
 assert converted==native,(idx,next((j,a,b) for j,(a,b) in enumerate(zip(converted,native)) if a!=b))
 print('PASS packed HD image',idx,w,h)
