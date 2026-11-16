"""Compare material/weight assignment with original 2x2 terrain triangulation."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import unicorn as U,pefile
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.1/Myth II.exe');base=p.OPTIONAL_HEADER.ImageBase
u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image())
s=0x10000000;u.mem_map(s,0x100000);sp=s+0xf000;stop=s+0xff00;descriptor=s+0x10000;vertices=s+0x20000;triangles=s+0x30000;out=s+0x40000
def alloc(uc,a,n,data):
 esp=u.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',u.mem_read(esp,4))[0];u.reg_write(UC_X86_REG_EAX,out);u.reg_write(UC_X86_REG_ESP,esp+4);u.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(U.UC_HOOK_CODE,alloc,begin=0x48265b,end=0x48265b)
cases=0
for variant in range(3):
 w=h=8;v=bytes((x+y*w)%7 for y in range(h) for x in range(w));t=bytes((i%5 if variant else 0) for i in range(w*h*2))
 if variant==2:v=bytes([1]*(w*h))
 u.mem_write(vertices,v);u.mem_write(triangles,t);u.mem_write(descriptor,struct.pack('<8I',w,h,0,vertices,0,triangles,0,0));u.mem_write(sp,struct.pack('<I',stop));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EDI,descriptor);u.emu_start(0x4299e3,stop,count=1000000);assert u.reg_read(UC_X86_REG_EIP)==stop
 words=struct.unpack('<'+str(w*h*2)+'I',u.mem_read(out,w*h*8))
 for y in range(h-1):
  for x in range(w-1):
   a=(x,y);b=(x+1,y);c=(x,y+1);d=(x+1,y+1)
   drawn=[(a,b,c),(b,d,c)] if (x^y)&1 else [(a,b,d),(a,d,c)]
   # Original triangle orientation and start corner recovered from 4299e3.
   bx=x//2*2;by=y//2*2;points=[(bx+dx,by+dy) for dy in range(3) for dx in range(3)]
   original={ (0,0):[(4,0,3),(4,1,0)],(1,0):[(4,2,1),(4,5,2)],(0,1):[(4,6,7),(4,3,6)],(1,1):[(4,8,5),(4,7,8)]}[(x%2,y%2)]
   for j,triangle in enumerate(drawn):
    native=j^(0 if x&1 else 1);word=words[(y*w+x)*2+native];override=t[(y*w+x)*2+(native^(y&1))]
    nativePoints=[points[k] for k in original[native]]
    for point in triangle:
     pos=nativePoints.index(point);materials=[]
     for k in range(3):
      idx=(word>>(9+7*k))&127
      if word&(1<<(k*3+pos)):materials.append(idx)
     expected=override or v[point[1]*w+point[0]]
     assert materials==([expected] if expected else []),(variant,x,y,j,point,materials,expected,hex(word))
     cases+=1
print('PASS',cases,'vertex material weights match native triangle descriptors')
Path('research/reports/detail-triangle-verification.json').write_text(json.dumps({'result':'pass','vertex_cases':cases,'reference':'Original 0x4299e3 / 0x429917 executed in isolated emulator'}))
