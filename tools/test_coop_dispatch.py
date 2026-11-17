"""Compiled VR dispatcher + original 471b DETACH serialization, offline only."""
import sys,struct,json
from pathlib import Path
sys.path[:0]=['tools/python-deps','tools']
import pefile,unicorn as U
from unicorn.x86_const import *
from inspect_film import parse
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE('build/integration-185/coop-dispatch-test.dll')
entry=d.OPTIONAL_HEADER.ImageBase+next(s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'coopExecute')
film=parse(Path('research/runtime/myth-1.8.5/recordings/Multiplay Coop Unit Handover.m2rec').read_bytes());record=next(c for c in film['commands'] if c['verb']==11)
payload=bytes.fromhex(record['payload_hex']);identity=struct.unpack_from('>H',payload,6)[0]
roster=json.loads(Path('research/reports/coop-live-roster.json').read_text());results=[]
for case in ['valid','return','third_member','enemy','dropped','observer','identity_changed','selection_changed','self','empty','invalid_slot','uncontrolled']:
 u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
 for pe in [p,d]:u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
 u.mem_map(0x1000000,0x40000)
 def w(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 mesh,actors,players,queue,buf,cmd,sp,stop=0x1000100,0x1001000,0x1005000,0x1009000,0x100a000,0x100b000,0x103f000,0x1000000
 w(0x6bee40,3);w(0x531618,mesh);w(mesh+8,7|(6<<16));w(0x532188,actors);w(0x532e6c,0x1008000);w(0x532284,2185)
 u.mem_write(actors,struct.pack('<H',identity));u.mem_write(0x532182,struct.pack('<H',1));u.mem_write(0x67ddb0,struct.pack('<HHH',0 if case=='empty' else 1,0,0))
 w(0x540a48,queue);w(queue,0,0,0,0,0,0,2048,buf);u.mem_write(buf,b'\xa5'*2048)
 rows=[bytearray.fromhex(h) for h in roster['players']];target=0 if case in ['return','self'] else 1;local=1 if case=='return' else 0
 if case=='third_member':rows[2]=bytearray(rows[1]);rows[2][:4]=struct.pack('<I',123456);target=2
 if case=='enemy':rows[1][10]=1
 if case=='dropped':rows[1][8]|=4
 if case=='observer':rows[1][8]|=64
 u.mem_write(players,b''.join(rows));w(0x53329c,players);u.mem_write(0x533296,struct.pack('<H',len(rows)));w(0x5332dc,local,0)
 expected_id=struct.unpack_from('<I',rows[target])[0];sig=((2166136261^identity)*16777619)&0xffffffff
 w(cmd,0x31444d43,1,23,32 if case=='invalid_slot' else target,expected_id+(case=='identity_changed'),0,0,sig+(case=='selection_changed'))
 w(sp,stop,cmd);u.reg_write(UC_X86_REG_ESP,sp);saved={UC_X86_REG_EBX:123,UC_X86_REG_ESI:456,UC_X86_REG_EDI:789,UC_X86_REG_EBP:321}
 for r,v in saved.items():u.reg_write(r,v)
 def intercept(c,a,n,data):
  if a not in [0x482b5e,0x4c9530,0x4c9488]:return
  stack=c.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',c.mem_read(stack,4))[0];c.reg_write(UC_X86_REG_EAX,0 if case=='uncontrolled' else 1);c.reg_write(UC_X86_REG_ESP,stack+4);c.reg_write(UC_X86_REG_EIP,ret)
 u.hook_add(U.UC_HOOK_CODE,intercept);u.emu_start(entry,stop,count=300000)
 assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
 for r,v in saved.items():assert u.reg_read(r)==v,(case,r)
 status=u.reg_read(UC_X86_REG_EAX);length=struct.unpack('<I',u.mem_read(queue+8,4))[0]
 if case in ['valid','return','third_member']:
  assert status==1,(case,status)
  got=bytes(u.mem_read(buf,length));assert got[8:]==struct.pack('>hhhH',0,target,1,identity),(case,got.hex())
 else:assert status in (18,19) and length==0,(case,status,length)
 assert bytes(u.mem_read(buf+length,16))==b'\xa5'*16
 results.append(dict(case=case,status=status,bytes=length))
Path('research/reports/coop-dispatch-tests.json').write_text(json.dumps(dict(cases=results,limitations='Original serializer and compiled dispatcher executed; selection eligibility and UI reset stubbed; no live network'),indent=2));print('PASS',len(results),'compiled dispatcher cases')
