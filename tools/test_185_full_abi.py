import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps');import pefile,unicorn as U
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE('build/integration-185/full-adapters-test.dll');rows=[]
for site,target in [(0x46b51c,0x46b629),(0x46b581,0x46b629),(0x45dbc1,0x4d0117),(0x45dbc6,0x49edf7),(0x4d1597,0x4d8f96)]:
 b=p.get_data(site-0x400000,5);assert b[0]==232 and site+5+struct.unpack_from('<i',b,1)[0]==target;rows.append({'callsite':hex(site),'target':hex(target)})
assert p.get_data(0x4d159c-0x400000,4)==bytes.fromhex('f6430e01')
for name,target,args,eax,expected in [('register_group',0x4cba6f,[7],0,[7]),('attack185',0x4cb3f4,[22,2],0,[22,2,0,0]),('script_camera_setup_hook',0x46b629,[0x1002000,1234],0x1003000,[0x1002000,1234])]:
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
 for pe in (p,d):u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
 u.mem_map(0x1000000,65536);sp=0x100f000;stop=0x1000000
 entry=d.OPTIONAL_HEADER.ImageBase+next(s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols if s.name.decode()==name)
 u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),stop,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EAX,eax);seen=[]
 def intercept(c,a,n,data):
  if a!=target:return
  ptr=c.reg_read(UC_X86_REG_ESP);actual=list(struct.unpack('<'+'I'*len(expected),c.mem_read(ptr+4,4*len(expected))));assert actual==expected,(name,actual)
  if name=='attack185':assert c.reg_read(UC_X86_REG_EAX)==0x67ddb0
  if name=='script_camera_setup_hook':assert c.reg_read(UC_X86_REG_EAX)==eax
  ret=struct.unpack('<I',c.mem_read(ptr,4))[0];c.reg_write(UC_X86_REG_ESP,ptr+4);c.reg_write(UC_X86_REG_EIP,ret);seen.append(a)
 u.hook_add(U.UC_HOOK_CODE,intercept);u.emu_start(entry,stop,count=100000)
 assert seen==[target] and u.reg_read(UC_X86_REG_ESP)==sp+4 and u.reg_read(UC_X86_REG_EIP)==stop
 rows.append({'adapter':name,'arguments':expected,'result':'pass'})
Path('research/reports/version-185-full-abi.json').write_text(json.dumps({'result':'pass','rows':rows,'scope':'Callsite signatures and compiled adapter argument forwarding; target functions stubbed'},indent=2));print('PASS 5 original hook sites and 3 compiled ABI adapters')
