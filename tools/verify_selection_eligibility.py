from verify_combat_builders import *
from itertools import product
c=machine();definition=0x1029000;c.mem_write(0x52a6f4,struct.pack('<I',definition));c.mem_write(ACTORS+2,bytes(2));cases=[]
for ab,flags,kind,active,game in product([0,8,16,32,56],[0,0x80000,0x2000000,0x2080000],[0,5],[0,1],[0,11,12]):
 c.mem_write(ACTORS+9,bytes([ab]));c.mem_write(definition,struct.pack('<I',flags));c.mem_write(definition+0x1b6,struct.pack('<h',kind));c.mem_write(0x67ec06,struct.pack('<HH',active,game))
 run(c,0x466c80,[],{UC_X86_REG_ECX:ACTORS});result=c.reg_read(UC_X86_REG_EAX)
 cases.append((ab,flags,kind,active,game,int(bool(result))))
source='#include "../src/bridge/selection_eligibility.h"\n#include <assert.h>\n#include <stdio.h>\nint main(){\n'
for a,f,k,active,g,want in cases:source+=f'assert(!!original_selection_eligible({a},{f},{k},{active},{g})=={want});\n'
source+=f'puts("PASS {len(cases)} selection eligibility cases against original x86");}}\n'
(ROOT/'build/test_selection_eligibility.c').write_text(source)
print('Original x86 cases:',len(cases))
