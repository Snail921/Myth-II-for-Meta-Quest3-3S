"""Offline x86 execution of Build 471b movement builder; no live writes."""
import sys,struct,json,hashlib
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as U
from unicorn.x86_const import *
from inspect_film import parse
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe')
f=Path('research/fixtures/films/capture-01.film').read_bytes();film=parse(f);command=next(x for x in film['commands'] if x['verb']==3);move=command['movement_candidate']
u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x1000000,0x40000)
def w(a,*values):u.mem_write(a,struct.pack('<'+'I'*len(values),*values))
actors,slots,opts,points,queue,bytes_=0x1001000,0x1002000,0x1002100,0x1002200,0x1002300,0x1003000
w(0x532188,actors);u.mem_write(actors+2*220,struct.pack('<H',move['unit_ids'][0]));u.mem_write(slots,struct.pack('<H',2));u.mem_write(opts,struct.pack('<hHH2x',move['flags'],move['formation_index'],move['final_facing_raw']));w(points,move['waypoints'][0]['raw_x'],move['waypoints'][0]['raw_y']);w(0x532284,film['game_parameters_candidate']['gameplay_version']);w(0x540a48,queue);w(queue,0,0,0,0,0,0,2048,bytes_);u.mem_write(bytes_,b'\xa5'*2048)
sp,stop=0x103f000,0x1000000
# New ABI: ECX=options; stack=(assignmentOptimization, mode, units, slots, pointsCount, points).
w(sp,stop,0,0xffffffff,1,slots,1,points);u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,opts)
try:u.emu_start(0x47b31c,stop,count=200000)
except Exception:print('Failed at',hex(u.reg_read(UC_X86_REG_EIP)));raise
assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
state=struct.unpack('<8I',u.mem_read(queue,32));generated=bytes(u.mem_read(bytes_,state[2]));assert generated[8:]==bytes.fromhex(command['payload_hex']),(generated.hex(),command['payload_hex']);assert generated[:8]==struct.pack('<hbbI',command['size'],3,-1,0);assert bytes(u.mem_read(bytes_+len(generated),16))==b'\xa5'*16
result={'result':'pass','builder':'0x47b31c','abi':'ECX options; stack assignmentOptimization, mode, units, slots, pointCount, points','actor_stride':220,'hex':generated.hex(),'matches_recorded_181_payload':True,'limitations':'assignment optimization disabled; synthetic state; no pathfinding, scheduling or live game commands'}
Path('research/reports/version-185-movement-builder.json').write_text(json.dumps(result,indent=2));print('PASS 471b movement builder generates exact legacy Film payload, preserves queue boundary and caller stack')

