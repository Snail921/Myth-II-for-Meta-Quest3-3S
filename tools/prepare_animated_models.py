from build_progress import tracked
"""Export every model shape referenced by this map's original anim tags."""
import json,struct
from pathlib import Path
from prepare_vr_assets import decode,u,s
ROOT=Path(__file__).resolve().parents[1]
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions
from terrain_lighting import lighting_settings,tint_rgba

def main():
 lighting=lighting_settings(FIXTURE)
 from tag_sources import inventory
 tags=inventory()
 def get(kind,name):
  path,t=tags[kind,name]
  with (ROOT/path).open('rb') as f:f.seek(t['offset']);return f.read(t['size'])
 records=json.loads((REPORT/'vr-models.json').read_text());ranges=[];offset=0;modeltags=set()
 for rec in records:
  if rec['animated_first_pose_only']:
   ranges.append((offset,rec['triangles']))
   anim=get('anim',rec['tag']);n=struct.unpack_from('>H',anim,4)[0]
   if not 0<n<=64:raise ValueError('anim shape count')
   for i in range(n):modeltags.add(anim[12+i*16:16+i*16].decode('mac_roman'))
  offset+=rec['triangles']
 textures=[];collections={};shapes={}
 for tag in tracked(sorted(modeltags),'アニメーションモデルを変換'):
  model=get('mode',tag);geom=model[4:8].decode('mac_roman');g=get('geom',geom)
  key=int.from_bytes(model[4:8],'big')
  if key in shapes:continue
  core=get('core',g[4:8].decode('mac_roman'));coll=core[:4].decode('mac_roman')
  if coll not in collections:
   ims,inst,seq=decode(get('.256',coll));collections[coll]=(len(textures),ims,inst,seq);textures.extend(ims)
  start,ims,inst,seq=collections[coll];d=u(g,76);mc,vc,sc=struct.unpack_from('>hhh',g,8);vo=d+u(g,40);so=d+u(g,52);mo=d+u(g,28);center=struct.unpack_from('>3h',g,16)
  if u(g,44)!=vc*6 or u(g,56)!=sc*64:raise ValueError('geometry strides')
  materials=[start+inst[seq[s(g,mo+i*64+32)][2][0][0]][3] for i in range(mc)]
  tris=[]
  for i in range(sc):
   p=so+i*64;mat=s(g,p+26)
   if not 0<=mat<mc:raise ValueError('material')
   vertices=[]
   for j in range(3):
    idx=s(g,p+32+j*8)
    if not 0<=idx<vc:raise ValueError('vertex')
    xyz=struct.unpack_from('>3h',g,vo+idx*6);uv=struct.unpack_from('>2H',g,p+28+j*8)
    vertices.extend([*(xyz[k]-center[k] for k in range(3)),uv[0]/65536,uv[1]/65536])
   tris.append((materials[mat],vertices))
  shapes[key]=tris
 with (OUT/'animated-models.pack').open('wb') as f:
  f.write(b'MYTHAM01');f.write(struct.pack('<3I',len(textures),len(shapes),len(ranges)))
  for a,b in ranges:f.write(struct.pack('<2I',a,b))
  for w,h,p in textures:f.write(struct.pack('<2I',w,h));f.write(tint_rgba(p,lighting))
  for key,tris in shapes.items():
   f.write(struct.pack('<2I',key,len(tris)))
   for tx,v in tris:f.write(struct.pack('<I15f',tx,*v))
 report=dict(shapes={hex(k):len(v) for k,v in shapes.items()},fallback_ranges=ranges,textures=len(textures))
 (REPORT/'animated-model-assets.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
if __name__=='__main__':main()
