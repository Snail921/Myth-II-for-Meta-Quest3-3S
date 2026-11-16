import sys,json,re
from pathlib import Path
sys.path.insert(0,'tools/python-deps');import pefile,capstone
r=json.loads(Path('research/reports/version-185-compatibility.json').read_text(encoding='utf8'));a=pefile.PE('research/extracted/update-1.8.1/Myth II.exe');b=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True;section=b.sections[0];new=section.get_data();base=0x400000+section.VirtualAddress
results=[]
for row in r['address_dependencies']:
 if row['kind']!='code':continue
 va=int(row['old_va'],16);allins=list(md.disasm(a.get_data(va-0x400000,200),va));best=None
 for count in (24,16,12,8):
  seq=allins[:count];raw=bytearray();mask=[]
  for i in seq:
   start=len(raw);raw.extend(i.bytes);mask.extend([True]*i.size)
   for off,size in ((i.imm_offset,i.imm_size),(i.disp_offset,i.disp_size)):
    val=int.from_bytes(i.bytes[off:off+size],'little') if size else 0
    if size==4 and (0x400000<=val<0x700000 or off==i.imm_offset and (i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL))) or off==i.imm_offset and val==216 and i.mnemonic in ('imul','add'):
     mask[start+off:start+off+size]=[False]*size
  pattern=b''.join(re.escape(bytes([v])) if m else b'.' for v,m in zip(raw,mask));matches=list(re.finditer(pattern,new,re.DOTALL))
  if len(matches)==1:best={'old':hex(va),'candidate':hex(base+matches[0].start()),'instructions':len(seq),'bytes':len(raw),'validated':False};break
 if best:results.append(best)
Path('research/reports/version-185-code-candidates.json').write_text(json.dumps(results,indent=2));print(json.dumps(results,indent=2))
