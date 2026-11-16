"""Find relocated data references using instruction-aligned masked code context; candidates only."""
from pathlib import Path
import sys,struct,re,json,collections
sys.path.insert(0,'tools/python-deps');import capstone,pefile
ROOT=Path(__file__).resolve().parents[1]
a=pefile.PE(str(ROOT/'research/extracted/update-1.8.1/Myth II.exe'));b=pefile.PE(str(ROOT/'research/extracted/update-1.8.5/Myth II.exe'))
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True;md.skipdata=True
sa=a.sections[0];sb=b.sections[0];oldbase=0x400000+sa.VirtualAddress;newbase=0x400000+sb.VirtualAddress;old=sa.get_data();new=sb.get_data()
report=json.loads((ROOT/'research/reports/version-185-compatibility.json').read_text(encoding='utf8'))
wanted={int(x['old_va'],16) for x in report['address_dependencies'] if x['kind']=='data'}
wanted.update([0x676140,0x675940,0x67f2bc,0x52a744,0x52f4e4,0x52f4e8,0x67ec38,0x529f20,0x52a9c0,0x529bd4,0x67ec00,0x67ec24,0x67383c,0x553fd8,0x52ae68,0x52ae6c])
ins=list(md.disasm(old,oldbase));hits=collections.defaultdict(list)
for n,i in enumerate(ins):
 if i.id==0:continue
 fields=[]
 for off,size in ((i.disp_offset,i.disp_size),(i.imm_offset,i.imm_size)):
  if size==4:
   value=int.from_bytes(i.bytes[off:off+4],'little')
   if value in wanted:fields.append((off,value))
 if not fields:continue
 for off,value in fields:
  start=max(0,n-4);seq=ins[start:n+9]
  if any(x.id==0 for x in seq):continue
  raw=bytearray();mask=[]
  for x in seq:
   s=len(raw);raw.extend(x.bytes);mask.extend([True]*x.size)
   for o,l in ((x.disp_offset,x.disp_size),(x.imm_offset,x.imm_size)):
    if l==4:
     v=int.from_bytes(x.bytes[o:o+4],'little')
     if 0x400000<=v<0x400000+a.OPTIONAL_HEADER.SizeOfImage or o==x.imm_offset and (x.group(capstone.CS_GRP_JUMP) or x.group(capstone.CS_GRP_CALL)):
      mask[s+o:s+o+l]=[False]*l
  pattern=b''.join(re.escape(bytes([c])) if m else b'.' for c,m in zip(raw,mask));found=list(re.finditer(pattern,new,re.DOTALL))
  if len(found)==1:
   pos=found[0].start()+i.address-seq[0].address+off;target=int.from_bytes(new[pos:pos+4],'little')
   if 0x52b000<=target<0x400000+b.OPTIONAL_HEADER.SizeOfImage:hits[value].append({'new':hex(target),'old_xref':hex(i.address),'new_xref':hex(newbase+pos-off)})
result={hex(v):{'candidates':dict(collections.Counter(x['new'] for x in rows)),'evidence':rows} for v,rows in hits.items()}
(ROOT/'research/reports/version-185-global-candidates.json').write_text(json.dumps(result,indent=2),encoding='utf8')
for v in sorted(wanted):print(hex(v),result.get(hex(v),{}).get('candidates',{}))

