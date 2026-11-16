"""Original pickup builder against the user's three dwarf pickup Film commands."""
import json,struct
from verify_combat_builders import machine,run,ROOT,ACTORS,IDS,OPTS,UC_X86_REG_ECX,UC_X86_REG_EDX
s=json.loads((ROOT/'research/reports/combat-films/Dwarf set and pick mines.json').read_text(encoding='utf-8'))
count=0
for cmd in s['commands']:
 if cmd['verb']!=6:continue
 d=cmd['semantic_candidate'];c=machine();c.mem_write(0x52a744,struct.pack('<I',2181))
 c.mem_write(ACTORS+2*216,struct.pack('<H',d['unit_ids'][0]));c.mem_write(IDS,struct.pack('<H',2))
 c.mem_write(OPTS,struct.pack('<H2xHH',d['flags'],d['object_index'],d['object_id']))
 result=run(c,0x446a17,[0xffffffff,OPTS],{UC_X86_REG_ECX:IDS,UC_X86_REG_EDX:1})
 assert result[8:]==bytes.fromhex(cmd['payload_hex'])
 count+=1
 print('PASS pickup Film tick',cmd['tick'],'object',d['object_index'],d['object_id'])
assert count==3
(ROOT/'research/reports/pickup-builder-verification.json').write_text(json.dumps({'result':'pass','film':'Dwarf set and pick mines','commands':count,'builder':'0x446a17','scope':'Original x86 builder executed with synthetic actor slots; pickup gameplay not emulated'},indent=2))
