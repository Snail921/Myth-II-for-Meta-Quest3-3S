import sys,struct
sys.path.insert(0,'tools/python-deps');import unicorn as U,pefile
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x10000000,0x10000)
stop,sp,mesh=0x10000100,0x10008000,0x10001000
u.mem_write(0x531618,struct.pack('<I',mesh))
def w(a,x,n=2):u.mem_write(a,x.to_bytes(n,'little'))
network=[]
def hook(uc,a,s,ctx):
 if a in (0x48e895,0x490be2,0x47ac3f):
  esp=uc.reg_read(UC_X86_REG_ESP)
  if a==0x490be2:uc.reg_write(UC_X86_REG_EAX,ctx['host'])
  if a==0x47ac3f:network.append(struct.unpack('<I',uc.mem_read(esp+4,4))[0])
  uc.reg_write(UC_X86_REG_EIP,struct.unpack('<I',uc.mem_read(esp,4))[0]);uc.reg_write(UC_X86_REG_ESP,esp+4)
ctx={'host':0};u.hook_add(U.UC_HOOK_CODE,hook,ctx)
def run(speed,up,mode=2,host=0,blocked=0):
 ctx['host']=host;w(0x6bee46,mode);w(0x6bee4e,speed);w(0x6644b4,speed,4);w(mesh+0x4e,blocked,1);w(0x532284,0x800,4);w(0x5332e0,0,4)
 u.mem_write(sp,struct.pack('<III',stop,up,0));u.reg_write(UC_X86_REG_ESP,sp);u.emu_start(0x48e795,stop,count=2000)
 assert u.reg_read(UC_X86_REG_EIP)==stop
 return int.from_bytes(u.mem_read(0x6bee4e,2),'little')
for a,b,c in [(5,0,4),(4,0,3),(3,0,0),(0,0,0),(0,1,3),(3,1,4),(4,1,5),(5,1,6),(10,1,11),(11,1,11)]:assert run(a,b)==c,(a,b,c)
for a in range(12):
 assert run(a,1,blocked=0x80)==a
 assert run(a,0,mode=1,host=0)==a
assert run(5,1,mode=1,host=1)==5 and network[-1]==6
print('PASS original 471b F1/F2 handler: half/quarter/stopped/resume, x64 cap, map restrictions, client rejection, host network command')
