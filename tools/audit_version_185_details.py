import sys,json,collections
from pathlib import Path
sys.path.insert(0,'tools/python-deps');sys.path.insert(0,'research/references/mythextract/scripts')
import capstone,pefile,myth_headers
root=Path('research/extracted');a=pefile.PE(str(root/'update-1.8.1/Myth II.exe'));b=pefile.PE(str(root/'update-1.8.5/Myth II.exe'));md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);md.detail=True
p=Path('research/reports/version-185-compatibility.json');r=json.loads(p.read_text(encoding='utf8'));evidence=collections.defaultdict(list)
for item in r['address_dependencies']:
 if item.get('candidate_count')!=1:continue
 x=int(item['old_va'],16);y=int(item['candidate_matches'][0],16)
 ia=list(md.disasm(a.get_data(x-0x400000,128),x))[:16];ib=list(md.disasm(b.get_data(y-0x400000,128),y))[:16]
 for u,v in zip(ia,ib):
  for off,size in ((u.disp_offset,u.disp_size),(u.imm_offset,u.imm_size)):
   if size==4:
    old=int.from_bytes(u.bytes[off:off+size],'little');new=int.from_bytes(v.bytes[off:off+size],'little')
    if 0x526000<=old<0x680000 and 0x52b000<=new<0x6c0000:evidence[hex(old)].append({'candidate':hex(new),'old_instruction':hex(u.address),'new_instruction':hex(v.address)})
r['data_address_candidates']=dict(evidence)
def tags(f):
 d=f.read_bytes();h=myth_headers.parse_mono_header(f.name,d);return {(str(t.tag_type),str(t.tag_id)):(str(t.name),d[t.tag_data_offset:t.tag_data_offset+t.tag_data_size]) for t in myth_headers.get_mono_tags(d,h)}
r['optional_plugin_comparisons']=[]
for pattern in ('*Maps','*Unity'):
 olds=sorted((root/'update-1.8.1/plugins').glob(pattern));old=olds[-1] if olds else None;new=sorted((root/'update-1.8.5/plugins').glob(pattern))[-1];x,y=tags(old) if old else {},tags(new);changes=[]
 for k in x.keys()|y.keys():
  if x.get(k)!=y.get(k):changes.append({'type':k[0],'id':k[1],'old_name':x.get(k,('',))[0],'new_name':y.get(k,('',))[0],'status':'added' if k not in x else 'removed' if k not in y else 'changed'})
 r['optional_plugin_comparisons'].append({'old':str(old),'new':str(new),'old_tags':len(x),'new_tags':len(y),'changes':changes})
p.write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf8')
print('DATA',json.dumps(dict(evidence),indent=2))
for i in r['optional_plugin_comparisons']:print(i['old'],i['new'],i['old_tags'],i['new_tags'],'changes',len(i['changes']),collections.Counter(c['type'] for c in i['changes']))
