from build_progress import tracked
"""Export original static model geometry; animated markers use first source pose only.
Pack v1 preserves source (vertical, horizontal) texture coordinates.
The renderer swaps them to OpenGL (u, v); generated connectors already store (u, v).
Live model permutation sync is separate.
"""
import json,struct,math,os
from pathlib import Path
from prepare_vr_assets import decode,u,s
ROOT=Path(__file__).resolve().parents[1]
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions
from terrain_lighting import lighting_settings,tint_rgba

def main():
 lighting=lighting_settings(FIXTURE)
 from tag_sources import inventory
 tags=inventory()
 def get(typ,tag):
  a,t=tags[(typ,tag)]
  with (ROOT/a).open('rb') as f:f.seek(t['offset']);return f.read(t['size'])
 mesh=json.loads((FIXTURE).read_text(encoding='utf-8'))
 textures=[];cache={};triangles=[];records=[]
 for marker in tracked(mesh['markers'],'モデル配置を変換'):
  if marker['type'] not in [6,11]:continue
  marker_tag=marker['tag'].encode('latin1').decode('mac_roman');tag=marker_tag;animated=marker['type']==11
  if animated:
   anim=get('anim',tag);tag=anim[12:16].decode('mac_roman')
  if tag=='\xff'*4:continue
  model=get('mode',tag);geomtag=model[4:8].decode('mac_roman');g=get('geom',geomtag);coretag=g[4:8].decode('mac_roman');core=get('core',coretag);colltag=core[:4].decode('mac_roman')
  if colltag not in cache:
   ims,instances,seqs=decode(get('.256',colltag));cache[colltag]=(len(textures),ims,instances,seqs);textures.extend(ims)
  start,ims,instances,seqs=cache[colltag];d=u(g,76);mc,vc,sc=struct.unpack_from('>hhh',g,8);vo=d+u(g,40);so=d+u(g,52);mo=d+u(g,28)
  if u(g,44)!=vc*6 or u(g,56)!=sc*64:raise ValueError('geometry stride')
  materials=[]
  permutation=u(model,52)+u(model,28)
  for i in range(mc):
   sequence=s(g,mo+i*64+32)
   if not 0<=sequence<len(seqs):raise ValueError('material sequence')
   # Use the geometry's base material; runtime model permutations are not yet exported.
   instance=instances[seqs[sequence][2][0][0]];materials.append(start+instance[3])
  # Original model orientation differs from actor heading: yaw 16384 is identity.
  yaw=(16384-marker['yaw'])/65536*math.tau;cos=math.cos(yaw);sin=math.sin(yaw)
  vertices=[]
  for i in range(vc):
   x,y,z=struct.unpack_from('>hhh',g,vo+i*6)
   cx,cy,cz=struct.unpack_from('>hhh',g,16);x-=cx;y-=cy;z-=cz
   vertices.append(((marker['pos_x']+cos*x-sin*y)/512,(marker['pos_y']+sin*x+cos*y)/512,(marker['pos_z']+z)/512))
  for i in range(sc):
   p=so+i*64;mat=s(g,p+26)
   if not 0<=mat<mc:raise ValueError('material index')
   tri=[]
   for j in range(3):
    v=s(g,p+32+j*8)
    if not 0<=v<vc:raise ValueError('vertex index')
    tu,tv=struct.unpack_from('>HH',g,p+28+j*8)
    x,y,z=vertices[v];tri.extend([x,y,z,tu/65536,tv/65536])
   triangles.append((materials[mat],tri))
  records.append({'marker_id':marker['id'],'tag':marker_tag,'model':tag,'geometry':geomtag,'triangles':sc,'animated_first_pose_only':animated})
 from prepare_connectors import append_connectors
 connector_report=append_connectors(mesh,get,textures,triangles,os.environ.get('MYTH_MAP_TAG') or (bytes.fromhex(FIXTURE.parent.name).decode('latin1') if FIXTURE.parent.parent.name=='levels' else None))
 (REPORT/'connectors.json').write_text(json.dumps(connector_report,indent=2),encoding='utf8')
 (OUT/'connectors.pack').write_bytes(struct.pack('<I',len(connector_report.get('ranges',[])))+b''.join(struct.pack('<8i',*r) for r in connector_report.get('ranges',[])))
 out=OUT/'models.pack'
 with out.open('wb') as f:
  f.write(b'MYTHMD01');f.write(struct.pack('<II',len(textures),len(triangles)))
  for w,h,p in textures:f.write(struct.pack('<II',w,h));f.write(tint_rgba(p,lighting))
  for tex,tri in triangles:f.write(struct.pack('<I15f',tex,*tri))
 (REPORT/'vr-models.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8');print(json.dumps({'models':len(records),'textures':len(textures),'triangles':len(triangles)}))
if __name__=='__main__':main()
