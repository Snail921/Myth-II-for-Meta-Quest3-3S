"""Execute compiled input-hook transactions; stub original selection/voice endpoints."""
import struct,json
from pathlib import Path
from verify_combat_builders import machine,ROOT,STOP,STACK,ACTORS,run,u,pefile
from unicorn.x86_const import *

c=machine()
dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'))
base=dll.OPTIONAL_HEADER.ImageBase
c.mem_map(base,(dll.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
c.mem_write(base,dll.get_memory_mapped_image())
entry=base+next(e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'MythTestExecute')
c.mem_write(0x67ec00,struct.pack("<H",3))
mesh=0x100b000
c.mem_write(0x529bd4,struct.pack('<I',mesh));c.mem_write(mesh+8,struct.pack('<HH',7,6))
c.mem_write(0x52a9f4,struct.pack('<I',0x100c000));c.mem_write(0x529f4e,struct.pack('<H',127))
for i in range(127):c.mem_write(ACTORS+i*216,struct.pack('<H',10000+i))
voice=[]
def selected():
 n=struct.unpack('<H',c.mem_read(0x553fd8,2))[0]
 return list(struct.unpack('<'+'H'*n,c.mem_read(0x553fdc,n*2)))
def original(cpu,address,size,user):
 if address not in (0x43b7a4,0x465107,0x467707,0x465f3c,0x43e3e7):return
 esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
 if address==0x43b7a4:cpu.reg_write(UC_X86_REG_EAX,int(cpu.reg_read(UC_X86_REG_EAX)<126))
 elif address==0x465107:c.mem_write(0x553fd8,struct.pack('<HH',0,0))
 elif address==0x467707:
  ids=selected();slot=cpu.reg_read(UC_X86_REG_EBX);assert slot not in ids
  ids.append(slot);c.mem_write(0x553fd8,struct.pack('<HH',len(ids),0));c.mem_write(0x553fdc,struct.pack('<'+'H'*len(ids),*ids))
 elif address==0x43e3e7:voice.append(selected())
 cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,original)
seq=0
def send(op,payload):
 global seq
 seq+=1;c.mem_write(0x1020000,struct.pack('<III',0x31444d43,seq,op)+payload)
 run(c,entry,[0x1020000],{})
 return c.reg_read(UC_X86_REG_EAX)
def begin(n,add=0):return send(9,struct.pack('<5I',n,add,0,0,0))
def chunk(ids):return send(10,struct.pack('<10H',*ids,*([0]*(10-len(ids)))))
def commit(n):return send(11,struct.pack('<5I',n,0,0,0,0))

assert chunk([10000])==19
assert begin(126)==1
for i in range(0,126,10):
 assert chunk(list(range(10000+i,10000+min(i+10,126))))==1
 assert selected()==[], 'BEGIN/CHUNK must not mutate game selection'
assert commit(126)==1 and selected()==list(range(126)) and len(voice)==1
assert begin(2)==1 and chunk([10001,10126])==1
assert commit(2)==18 and selected()==list(range(126)), 'Unauthorized actor must preserve selection'
assert begin(2)==1 and chunk([10001])==1 and commit(2)==19
assert selected()==list(range(126))
assert begin(1)==1 and chunk([10001,10002])==19
assert begin(127)==16
assert begin(1)==1 and chunk([10002])==1 and commit(1)==1 and selected()==[2]
assert begin(3,1)==1 and chunk([10002,10003,10003])==1 and commit(3)==1 and selected()==[2,3]
assert begin(0)==1 and commit(0)==1 and selected()==[]
print('PASS compiled group transaction: 126 units, atomic commit, ownership rejection, truncation, overflow, additive deduplication, empty clear')
(ROOT/'research/reports/group-selection-verification.json').write_text(json.dumps(dict(result='pass',max_group=126,original_selection_and_voice_endpoints_stubbed=True),indent=2))
