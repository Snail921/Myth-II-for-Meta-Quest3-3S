import sys,json,struct
from pathlib import Path
sys.path.insert(0,'tools')
from plugin_sources import archive
from prepare_map_fixture import decode,schemas
S=schemas(); result=[]
for p in Path('Myth II - Plugins').rglob('*'):
 if not p.is_file():continue
 with p.open('rb') as f:h=f.read(128)
 if len(h)!=128 or h[124:128]!=b'dng2' or struct.unpack_from('>H',h)[0]!=1:continue
 a=archive(p); tags={(t['type'],t['id']):t for t in a['tags']}
 def raw(t):
  with p.open('rb') as f:f.seek(t['offset']);return f.read(t['size'])
 rows=[]
 for e in a['entries']:
  t=tags.get(('mesh',e['id']))
  if not t:continue
  hdr=decode(S['MeshHeaderFmt'],raw(t)); key=hdr['pregame_storyline_tag'].encode('latin1').decode('mac_roman');j=tags.get(('text',key))
  journal=raw(j).decode('mac_roman',errors='replace') if j else ''
  
  if hdr['flags']==0xd0000 and e['name']=='Willow Creek':continue # disabled original-campaign placeholders
  rows.append(dict(**e,key=t['id_hex'],mesh_name=t['name'],header=hdr,journal=journal))
 result.append(dict(path=str(p),name=p.name,entries=rows,tag_count=len(tags)))
Path('build/plugin-skybox-inventory.json').write_text(json.dumps(result,ensure_ascii=True,indent=2),encoding='utf8')
for a in result:
 print(a['name'],len(a['entries']))
 for e in a['entries']:print(' ',e['key'],repr(e['name']), 'journal',len(e['journal']))
