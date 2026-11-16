"""Read-only 1.8.1/1.8.5 compatibility inventory. Candidate matches are NOT verified ports."""
from pathlib import Path
import sys,re,json,hashlib,collections
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools/python-deps'),str(ROOT/'research/references/mythextract/scripts')]
import pefile,capstone,myth_headers
old=ROOT/'research/extracted/update-1.8.1';new=ROOT/'research/extracted/update-1.8.5'
def pe(folder):return pefile.PE(str(folder/'Myth II.exe'))
a,b=pe(old),pe(new);base=a.OPTIONAL_HEADER.ImageBase
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True
sources=list((ROOT/'src/input').glob('*'))+list((ROOT/'src/bridge').glob('*'))+[ROOT/'src/standalone/StandaloneBridge.java']
addresses=collections.defaultdict(set)
for p in sources:
 if p.is_file():
  for value in re.findall(r'0x[0-9a-fA-F]+',p.read_text(encoding='utf8')):
   v=int(value,16)
   if base<=v<base+a.OPTIONAL_HEADER.SizeOfImage:addresses[v].add(str(p.relative_to(ROOT)))
def section(pe,va):
 for s in pe.sections:
  if s.VirtualAddress<=va-pe.OPTIONAL_HEADER.ImageBase<s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData):return s

def read(pe,va,n):
 try:return pe.get_data(va-pe.OPTIONAL_HEADER.ImageBase,n)
 except:return b''
text=next(s for s in b.sections if s.Characteristics&0x20);data=text.get_data();textbase=b.OPTIONAL_HEADER.ImageBase+text.VirtualAddress
rows=[]
for va,files in sorted(addresses.items()):
 sec=section(a,va);code=bool(sec and sec.Characteristics&0x20);r={'old_va':hex(va),'source_files':sorted(files),'kind':'code' if code else 'data','same_address_first16_equal':read(a,va,16)==read(b,va,16)}
 if code:
  ins=list(md.disasm(read(a,va,128),va))[:16];raw=bytearray();mask=[]
  for i in ins:
   start=len(raw);raw.extend(i.bytes);mask.extend([True]*i.size)
   # Mask absolute addresses and relative control-flow targets, preserve structure offsets/constants.
   for off,size in ((i.imm_offset,i.imm_size),(i.disp_offset,i.disp_size)):
    if size==4:
     val=int.from_bytes(i.bytes[off:off+size],'little')
     if base<=val<base+a.OPTIONAL_HEADER.SizeOfImage or (off==i.imm_offset and (i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL))):
      for j in range(start+off,start+off+size):mask[j]=False
  pattern=b''.join(re.escape(bytes([c])) if keep else b'.' for c,keep in zip(raw,mask))
  matches=[textbase+m.start() for m in re.finditer(pattern,data,re.DOTALL)] if len(raw)>=24 else []
  r.update(candidate_matches=[hex(x) for x in matches[:12]],candidate_count=len(matches),signature_bytes=len(raw),old_instructions=[i.mnemonic+' '+i.op_str for i in ins[:4]])
 rows.append(r)

def tags(folder):
 result={};archives=[]
 for p in sorted((folder/'plugins').glob('*')):
  raw=p.read_bytes()
  try:h=myth_headers.parse_mono_header(p.name,raw)
  except Exception as e:archives.append({'name':p.name,'error':str(e)});continue
  ts=myth_headers.get_mono_tags(raw,h);archives.append({'name':p.name,'type':h.type.name,'count':len(ts),'sha256':hashlib.sha256(raw).hexdigest()})
  if h.type.name!='PATCH':continue
  for t in ts:
   key=(str(t.tag_type),str(t.tag_id));blob=raw[t.tag_data_offset:t.tag_data_offset+t.tag_data_size]
   result[key]={'name':str(t.name),'archive':p.name,'sha256':hashlib.sha256(blob).hexdigest(),'size':len(blob)}
 return result,archives
at,aa=tags(old);bt,ba=tags(new)
changes=[]
for key in sorted(at.keys()|bt.keys()):
 if key not in at:status='added_patch_tag'
 elif key not in bt:status='removed_patch_override'
 elif at[key]['sha256']!=bt[key]['sha256']:status='changed'
 else:continue
 changes.append({'type':key[0],'id':key[1],'status':status,'old':at.get(key),'new':bt.get(key)})
report={'method':'Static inspection only; address literals include comments/constants. Masked instruction candidates require semantic and calling-convention validation. Patch tags compared in lexical archive order; not a verified active runtime resolver.','binaries':[], 'address_dependencies':rows,'old_archives':aa,'new_archives':ba,'patch_tag_changes':changes}
for folder,p in ((old,a),(new,b)):
 report['binaries'].append({'path':str((folder/'Myth II.exe').relative_to(ROOT)),'sha256':hashlib.sha256((folder/'Myth II.exe').read_bytes()).hexdigest(),'machine':hex(p.FILE_HEADER.Machine),'base':hex(p.OPTIONAL_HEADER.ImageBase),'image_size':p.OPTIONAL_HEADER.SizeOfImage,'sections':[{'name':s.Name.decode().strip('\0'),'rva':hex(s.VirtualAddress),'size':s.Misc_VirtualSize} for s in p.sections]})
report['summary']={'literal_addresses':len(rows),'code_literals':sum(r['kind']=='code' for r in rows),'unique_code_candidates':sum(r.get('candidate_count')==1 for r in rows),'same_address_code_first16':sum(r['kind']=='code' and r['same_address_first16_equal'] for r in rows),'patch_changes_by_type':dict(collections.Counter(c['type'] for c in changes))}
(ROOT/'research/reports/version-185-compatibility.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf8')
print(json.dumps(report['summary'],indent=2));print('guard',read(a,0x46bf56,4).hex(),read(b,0x46bf56,4).hex())

