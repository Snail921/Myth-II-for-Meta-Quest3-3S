"""Execute candidate read-only eligibility routine in Unicorn; never execute inside the live game."""
from pathlib import Path
import sys,json,struct
sys.path[:0]=['tools/python-deps','tools']
import unicorn as U
from unicorn.x86_const import *
from probe_engine_state import Memory
m=Memory(int(sys.argv[1]));assert m.profile['id']=='1.8.5-471b'
try:
 s=m.snapshot();assert s['consistent_tick'] and s['same_mesh']
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);pages={};faults=[];writes=[]
 def load(uc,access,address,size,value,data):
  page=address&~4095
  if page in pages:return False
  try:raw=m.read(page,4096)
  except Exception as e:faults.append({'address':hex(address),'error':str(e)});return False
  uc.mem_map(page,4096);uc.mem_write(page,raw);pages[page]=True;return True
 u.hook_add(U.UC_HOOK_MEM_UNMAPPED,load)
 stack=0x7f000000;stop=0x7f001000;u.mem_map(stack,8192)
 def onwrite(uc,access,address,size,value,data):
  if not stack<=address<stop:writes.append(hex(address))
 u.hook_add(U.UC_HOOK_MEM_WRITE,onwrite)
 results=[]
 for actor in s['units']:
  sp=stack+4000;u.mem_write(sp,struct.pack('<I',stop))
  for reg in (UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EAX,actor['slot'])
  u.emu_start(0x482b5e,stop,count=100000)
  if u.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Routine did not return')
  if u.reg_read(UC_X86_REG_ESP)!=sp+4:raise ValueError('Unexpected stack convention')
  results.append({'slot':actor['slot'],'id':actor['identity'],'eligible':u.reg_read(UC_X86_REG_EAX)&1})
 assert not writes,writes
 report={'profile':m.profile['id'],'routine':'0x482b5e','source':'read-only live memory pages; immutable within each cached page','limitations':'Pages were fetched lazily, not an atomic whole-game snapshot; eligibility only, not command injection','rows':results,'emulated_writes_outside_stack':writes,'faults':faults,'selected':s['selection']}
 Path('research/reports/version-185-control-emulation.json').write_text(json.dumps(report,indent=2))
 print('PASS eligible routine returns for',len(results),'actors; eligible',sum(r['eligible'] for r in results),'selected',s['selection']['slots'],'non-stack writes',len(writes))
finally:m.close()
