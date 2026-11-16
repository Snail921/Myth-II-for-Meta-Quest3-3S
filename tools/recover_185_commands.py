import sys,re,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps');import capstone,pefile
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True
p=pefile.PE('research/extracted/update-1.8.1/Myth II.exe');q=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');data=q.sections[0].get_data();base=0x401000
out={}
for va in (0x446764,0x4481d7,0x468e13,0x446948,0x446a17,0x46823e,0x44c631,0x465f3c,0x467c3b,0x467eec,0x468ae7,0x4a03c7,0x4321f1,0x4321f6,0x43e529,0x43e55f,0x46535c,0x465906):
 ins=list(md.disasm(p.get_data(va-0x400000,700),va));matches=[]
 for start in range(0,len(ins)-8,4):
  raw=bytearray();mask=[]
  for i in ins[start:start+8]:
   n=len(raw);raw.extend(i.bytes);mask.extend([True]*i.size)
   for off,size in ((i.imm_offset,i.imm_size),(i.disp_offset,i.disp_size)):
    val=int.from_bytes(i.bytes[off:off+size],'little')
    if size==4 and (0x400000<=val<0x700000 or off==i.imm_offset and (i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL))) or off==i.imm_offset and val==216 and i.mnemonic in ('imul','add'):mask[n+off:n+off+size]=[False]*size
  if len(raw)<22:continue
  pattern=b''.join(re.escape(bytes([c])) if k else b'.' for c,k in zip(raw,mask));m=list(re.finditer(pattern,data,re.DOTALL))
  if len(m)==1:
   at=m[0].start();pro=data.rfind(b'\x55\x8b\xec',max(0,at-1600),at+1);matches.append({'old_site':hex(ins[start].address),'new_site':hex(base+at),'preceding_prologue':hex(base+pro)})
 out[hex(va)]=matches;print(hex(va),matches[:12])
Path('research/reports/version-185-command-candidates.json').write_text(json.dumps(out,indent=2))
