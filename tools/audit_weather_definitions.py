"""Audit original weather definitions without relying on level names."""
import json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from tag_sources import inventory
from plugin_sources import archive
rows=[];lightning=[];sources={}
for (kind,tag),(path,t) in inventory().items():
 if kind in ('part','ligh'):sources[path]=None
for item in json.loads(Path('build/plugin-skybox-inventory.json').read_text()):sources[item['path']]=None
for path in sources:
 a=archive(path)
 with Path(path).open('rb') as f:
  for t in a['tags']:
   if t['type'] not in ('part','ligh'):continue
   f.seek(t['offset']);b=f.read(t['size'])
   if t['type']=='ligh':lightning.append(dict(archive=path,id=t['id'],name=t['name']));continue
   rows.append(dict(archive=path,id=t['id'],name=t['name'],core=b[4:8].decode('latin1'),sequence=struct.unpack_from('>H',b,20)[0],count=struct.unpack_from('>H',b,22)[0],scale=struct.unpack_from('>H',b,26)[0]/256,tile=struct.unpack_from('>H',b,92)[0],height=struct.unpack_from('>hh',b,94),flags=struct.unpack_from('>I',b)[0]))
report=dict(weather=rows,lightning_tags=lightning,scope='Foundation/PATCH and owned gameplay PLUGIN archives; part is shared precipitation system; ligh is separate projectile lightning, not a fullscreen weather overlay')
Path('research/reports/weather-definition-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
print('weather definitions',len(rows),'cores',sorted(set(r['core'] for r in rows)))
print('lightning tags',len(lightning),sorted(set(r['name'] for r in lightning)))
