"""Offline x86 execution of Build 471b movement builder; no live writes."""
import sys,struct,json,hashlib
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as U
from unicorn.x86_const import *
from inspect_film import parse
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe')

film=parse(Path('research/runtime/myth-1.8.5/recordings/Multiplay Coop Unit Handover.m2rec').read_bytes())
results=[]
for command in film['commands']:
 if command['verb']!=11:continue
 payload=bytes.fromhex(command['payload_hex']);flags,target,count=struct.unpack_from('>hhh',payload);ids=struct.unpack_from('>'+str(count)+'H',payload,6)
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x1000000,0x40000)
 def w(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 actors,slots,opts,queue,buf=0x1001000,0x1002000,0x1002100,0x1002300,0x1003000
 w(0x532188,actors);w(0x532284,2185);w(0x540a48,queue);w(queue,0,0,0,0,0,0,2048,buf);u.mem_write(buf,b'\xa5'*2048)
 for i,identity in enumerate(ids):u.mem_write(actors+i*220,struct.pack('<H',identity))
 u.mem_write(slots,struct.pack('<'+str(count)+'H',*range(count)));u.mem_write(opts,struct.pack('<hh',flags,target))
 sp,stop=0x103f000,0x1000000;w(sp,stop,0xffffffff,opts);u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,slots);u.reg_write(UC_X86_REG_EDX,count)
 u.emu_start(0x47b201,stop,count=200000)
 assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
 size=struct.unpack('<8I',u.mem_read(queue,32))[2];got=bytes(u.mem_read(buf,size));assert got[8:]==payload,(got.hex(),payload.hex())
 assert got[:8]==struct.pack('<hbbI',command['size'],11,-1,0)
 assert bytes(u.mem_read(buf+size,16))==b'\xa5'*16
 results.append(dict(tick=command['tick'],target=target,count=count,matched=True))
print(results)
Path('research/reports/coop-detach-builder.json').write_text(json.dumps(dict(builder='0x47b201',abi='ECX slots, EDX count, stack mode/options',results=results,limitations='Offline original-x86 command serialization only; not live network validation'),indent=2))
