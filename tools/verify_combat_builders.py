"""Compare original Build 434 command builders with captured combat Films.

Uses synthetic slots, not live process memory. Special-key dispatcher tests stub
ability availability and presentation calls; they do not validate actor gameplay.
"""
from pathlib import Path
import json, struct, sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile
import unicorn as u
from unicorn.x86_const import *
from inspect_film import parse
from inventory_combat_films import decode

raw=(ROOT/'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
pe=pefile.PE(data=raw)
STOP=0x1000000
STACK=0x101f000
ACTORS=0x1001000
IDS=0x1007000
TARGETS=0x1007200
OPTS=0x1007400
POINTS=0x1007600
QUEUE=0x1008000
BYTES=0x1009000

def machine():
 c=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
 c.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 c.mem_write(0x400000,pe.get_memory_mapped_image())
 c.mem_map(STOP,0x30000)
 c.mem_write(0x529f54,struct.pack('<I',ACTORS))
 c.mem_write(0x52f4e4,struct.pack('<I',QUEUE))
 c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES))
 c.mem_write(BYTES,b'\xa5'*2048)
 return c

def slots(c,ids,where,start):
 indices=[]
 for i,identifier in enumerate(ids,start):
  c.mem_write(ACTORS+i*216,struct.pack('<h',identifier));indices.append(i)
 c.mem_write(where,struct.pack('<'+'h'*len(indices),*indices))

def run(c,entry,args,registers):
 c.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
 c.reg_write(UC_X86_REG_ESP,STACK)
 for reg,value in registers.items():c.reg_write(reg,value)
 c.emu_start(entry,STOP,count=200000)
 assert c.reg_read(UC_X86_REG_EIP)==STOP
 assert c.reg_read(UC_X86_REG_ESP)==STACK+4
 size=struct.unpack('<7I',c.mem_read(QUEUE,28))[2]
 assert bytes(c.mem_read(BYTES+size,16))==b'\xa5'*16
 return bytes(c.mem_read(BYTES,size))

def main():
 results=[]
 for path in sorted((ROOT/'research/fixtures/films/combat-2026-11-08').glob('*.film')):
  if path.stem=='Last Recording':continue
  film=parse(path.read_bytes())
  for command in film['commands']:
   verb=command['verb']
   if verb not in (2,3,4,5):continue
   c=machine();d=decode(command);n=len(d['unit_ids'])
   slots(c,d['unit_ids'],IDS,3)
   c.mem_write(0x52a744,struct.pack('<I',film['game_parameters_candidate']['gameplay_version']))
   regs={UC_X86_REG_ECX:IDS,UC_X86_REG_EDX:n}
   if verb==2:
    payload=bytes.fromhex(command['payload_hex'])
    c.mem_write(OPTS,struct.pack('<hh',*struct.unpack('>hh',payload[:4])))
    entry,args=0x446543,[0xffffffff,OPTS]
   elif verb==3:
    c.mem_write(OPTS,struct.pack('<4H',*struct.unpack('>4H',bytes.fromhex(command['payload_hex'])[:8])))
    pts=d['waypoints'];c.mem_write(POINTS,b''.join(struct.pack('<ii',p['raw_x'],p['raw_y']) for p in pts))
    entry,args=0x446764,[0xffffffff,OPTS,n,len(pts),POINTS]
   elif verb==4:
    slots(c,d['target_ids'],TARGETS,150)
    c.mem_write(OPTS,struct.pack('<H',d['flags']))
    entry,args=0x446848,[0xffffffff,OPTS,n,len(d['target_ids']),TARGETS]
   else:
    c.mem_write(OPTS,struct.pack('<H2xiii',d['flags'],*d['raw_position']))
    entry,args=0x446948,[0xffffffff,OPTS]
   generated=run(c,entry,args,regs)
   assert generated[:8]==struct.pack('<hbbI',command['size'],verb,-1,0),(path,command['tick'])
   assert generated[8:]==bytes.fromhex(command['payload_hex']),(path,command['tick'])
   results.append(dict(film=path.stem,tick=command['tick'],verb=verb,builder=hex(entry),payload_equal=True))
 # Execute the original T action dispatcher with deterministic ability classes.
 special=[]
 for ability in (-1,0,1):
  c=machine();slots(c,[10090],0x553fdc,3)
  c.mem_write(0x553fd8,struct.pack('<HH',1,0));c.mem_write(0x67ec06,struct.pack('<h',2))
  c.mem_write(0x553f22,struct.pack('<h',-1));c.mem_write(0x52a744,struct.pack('<I',0x6a4))
  presentation={0x4be409,0x4200d3,0x4807c4,0x480d44,0x48081d,0x465e94}
  def stub(cpu,address,size,user):
   if address not in presentation and address!=0x43e7d3:return
   esp=cpu.reg_read(UC_X86_REG_ESP)
   ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
   cpu.reg_write(UC_X86_REG_EAX,ability&0xffff if address==0x43e7d3 else 0)
   cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
  c.hook_add(u.UC_HOOK_CODE,stub)
  generated=run(c,0x465f3c,[3],{})
  mode=struct.unpack('<h',c.mem_read(0x553f22,2))[0]
  assert (len(generated)>0)==(ability==0)
  assert mode==(3 if ability==1 else -1)
  if ability==0:assert generated[8:]==bytes.fromhex('000000030001276a')
  special.append(dict(ability_class=ability,queued_bytes=len(generated),original_cursor_mode=mode))
 voices=[]
 for kind in (0,3,5):
  for count in (1,4):
   c=machine();slots(c,list(range(10000,10000+count)),IDS,3)
   emitted=[]
   def sound_stub(cpu,address,size,user):
    if address!=0x43cd55:return
    esp=cpu.reg_read(UC_X86_REG_ESP)
    ret,voice=struct.unpack('<II',cpu.mem_read(esp,8));emitted.append(voice)
    cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
   c.hook_add(u.UC_HOOK_CODE,sound_stub)
   run(c,0x43e3e7,[count,kind],{UC_X86_REG_EBX:IDS})
   expected=kind+(1 if count>2 else 0)
   assert emitted==[expected]*count,(kind,count,emitted)
   voices.append(dict(kind=kind,count=count,original_voice_requests=emitted))
 report=dict(result='pass',film_command_count=len(results),commands=results,special_dispatch= special,voice_dispatch=voices,
             limitations='Synthetic slots; presentation/ability query stubbed only for special dispatcher. Live combat and radial UI need device validation.')
 (ROOT/'research/reports/combat-builder-verification.json').write_text(json.dumps(report,indent=2))
 print('PASS',len(results),'original builder payloads match Film; 3 special dispatcher branches; 6 voice dispatch cases')

if __name__=='__main__':main()
