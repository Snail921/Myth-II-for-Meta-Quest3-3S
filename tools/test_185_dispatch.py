"""Test compiled dispatch with original movement serialization, other calls stubbed.
Never injects or writes into any running game.
"""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import unicorn as U,pefile
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE('build/integration-185/dispatcher_test.dll')
entry=d.OPTIONAL_HEADER.ImageBase+next(s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'integrationExecute')
results=[]
for units in (1,4,32,126):
 for op in (3,5,6,7,12,16,17,18,19,20,21):
  u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
  for pe in (p,d):u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
  u.mem_map(0x1000000,0x40000)
  def w(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
  mesh,actors,queue,buf,command,sp,stop=0x1000100,0x1001000,0x1009000,0x100a000,0x100b000,0x103f000,0x1000000
  w(0x6bee40,3);w(0x531618,mesh);w(mesh+8,7|(6<<16));w(0x532188,actors);w(0x532e6c,0x1008000)
  u.mem_write(0x67ddb0,struct.pack('<HH',units,0));u.mem_write(0x532182,struct.pack('<H',units))
  for i in range(units):u.mem_write(actors+i*220,struct.pack('<H',10000+i));u.mem_write(0x67ddb4+i*2,struct.pack('<H',i))
  w(0x532284,2181);w(0x540a48,queue);w(queue,0,0,0,0,0,0,4096,buf);u.mem_write(buf,b'\xa5'*4096)
  calls=[]
  def intercept(c,a,n,data):
   if a not in (0x482b5e,0x4cc617,0x4858c5,0x4c9530):return
   calls.append(a);esp=c.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',c.mem_read(esp,4))[0]
   c.reg_write(UC_X86_REG_EAX,1 if a==0x482b5e else 3 if a==0x4cc617 else 0);c.reg_write(UC_X86_REG_ESP,esp+4);c.reg_write(UC_X86_REG_EIP,ret)
  u.hook_add(U.UC_HOOK_CODE,intercept)
  w(command,0x31444d43,1,op,0,0,90000,78000,0);w(sp,stop,command);u.reg_write(UC_X86_REG_ESP,sp)
  saved={UC_X86_REG_EBX:123,UC_X86_REG_ESI:456,UC_X86_REG_EDI:789,UC_X86_REG_EBP:321}
  for r,v in saved.items():u.reg_write(r,v)
  u.emu_start(entry,stop,count=300000)
  assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
  for r,v in saved.items():assert u.reg_read(r)==v
  status=u.reg_read(UC_X86_REG_EAX);length=struct.unpack('<I',u.mem_read(queue+8,4))[0]
  if op==3:assert status==1 and length>8 and 0x4858c5 in calls
  else:assert status==17 and length==0 and not calls
  results.append({'units':units,'op':op,'status':status,'command_bytes':length})
Path('research/reports/version-185-dispatch-test.json').write_text(json.dumps({'result':'pass','cases':results,'limitations':'Engine control/voice/formation getter/cancel calls stubbed; original movement builder executed. Does not prove live UI calls.'},indent=2))
print('PASS',len(results),'compiled dispatcher cases; unsupported commands never enter engine calls')
