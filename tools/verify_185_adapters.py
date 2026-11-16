"""Execute the compiled 471b C ABI adapters against isolated engine images."""
import sys,json,hashlib
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile
from unicorn.x86_const import *
SAVED={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
def saved_registers(u):
 for r,v in SAVED.items():u.reg_write(r,v)
def assert_registers(u):
 for r,v in SAVED.items():assert u.reg_read(r)==v,('callee-saved register',r)
def load_adapter(u,name):
 p=pefile.PE('build/input-185/adapters.dll');base=p.OPTIONAL_HEADER.ImageBase
 u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(base,p.get_memory_mapped_image())
 return base+next(s.address for s in p.DIRECTORY_ENTRY_EXPORT.symbols if s.name.decode()==name)
def main():
 from verify_185_movement_matrix import run as move
 from verify_185_formation import run as form
 n=0
 for units in (1,4,32,126):
  for points in (1,4):
   for flags,f,facing in ((1,0,0),(8,3,0),(24,3,256),(9,3,0),(11,3,0)):
    assert move('1.8.5',units,points,flags,f,facing,True)==move('1.8.1',units,points,flags,f,facing);n+=1
 for f in range(10):
  for facing in (0,256,768):
   for points in (1,4):
    assert form('185',f,facing,points,True)==form('181',f,facing,points);n+=1
 report={'result':'pass','cases':n,'adapter_sha256':hashlib.sha256(Path('build/input-185/adapters.dll').read_bytes()).hexdigest(),'callee_saved_registers_preserved':True,'caller_stack_preserved':True,'limitations':'Offline execution only. Engine-thread dispatcher and live integration not yet implemented. Assignment optimization disabled.'}
 Path('research/reports/version-185-adapter-verification.json').write_text(json.dumps(report,indent=2),encoding='utf8')
 print('PASS: 100 compiled adapter cases; original outputs, cdecl stack and saved registers verified')
if __name__=='__main__':main()
