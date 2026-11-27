"""Build isolated original map packs; no running game or fixed runtime slots required."""
import os,sys,json,struct,subprocess,shutil,hashlib,math
from pathlib import Path
from level_assets import ROOT,inventory,raw_tag,dependencies,parse_mesh,schemas
from prepare_vr_assets import u
from prepare_particle_assets import signature
from campaign_levels import campaign,identity
import threading
from build_progress import progress,tracked
dependency_lock=threading.Lock()

def collection_fields(data):
 fields=tuple(u(data,x) for x in (64,96,112,128,252));h=2166136261;start=u(data,248)+u(data,132)
 for i in range(u(data,128)):
  p=start+i*128;row=data[p:p+64].lower()+struct.pack('<2I',u(data,p+64),u(data,p+68))
  for v in row:h=((h^v)*16777619)&0xffffffff
 return (*fields,h)

def build(mapid,name,canonical,tags):
 tagged=bool(os.environ.get('MYTH_TAG_PROFILE'))
 key=mapid.encode('latin1').hex();work=Path(os.environ.get("MYTH_LEVELS_ROOT",str(ROOT/"build/levels")))/key;scene=work/'scene';out=work/'assets';report=work/'reports'
 for d in (scene,out,report):d.mkdir(parents=True,exist_ok=True)
 raw=raw_tag(tags,'mesh',mapid);mesh=parse_mesh(raw,schemas());(work/'decoded-map.json').write_text(json.dumps(mesh,indent=2),encoding='utf8')
 progress('依存素材を展開',item=name)
 with dependency_lock:deps,missing=dependencies(tags,mapid)
 # Invalid sentinel references and absent optional detail maps are normal in these originals.
 real_missing=[x for x in missing if x[1]!='\u02c7'*4 and x[0]!='dmap']
 if real_missing:raise ValueError(real_missing)
 model_collections=set()
 for (kind,tag),data in deps.items():
  if kind=='geom':
   core=raw_tag(tags,'core',data[4:8].decode('mac_roman'));model_collections.add(core[:4].hex())
 assets=[];keys={}
 for (kind,tag),data in tracked(sorted(deps.items()),'素材コレクションを抽出'):
  if kind!='.256' or not u(data,128):continue
  fields=collection_fields(data)
  if not tagged and fields in keys:
   if keys[fields]!=data:raise ValueError('Ambiguous live collection key')
   continue
  keys[fields]=data
  taghex=tags[kind,tag][1]['id_hex'];slot=canonical.setdefault(taghex,max([31,*canonical.values()])+1)
  if slot>=255:raise ValueError('Canonical sprite capacity')
  path='collection-'+taghex+'.bin';(scene/path).write_bytes(data)
  assets.append(dict(runtime_index=slot,file=path,name=tags[kind,tag][1]['name'],tag_id_hex=taghex,fields=fields,model_material=taghex in model_collections))
 # Engine-created effects (including world-knot teleportation) have no map dependency.
 for tag in ('fire','effe','buco','ebbf','mafi','guts','wate','teef'):
  if ('.256',tag) not in deps and ('.256',tag) in tags:
   data=raw_tag(tags,'.256',tag);taghex=tags['.256',tag][1]['id_hex'];slot=canonical.setdefault(taghex,max([31,*canonical.values()])+1);path='collection-'+taghex+'.bin';(scene/path).write_bytes(data);assets.append(dict(runtime_index=slot,file=path,name=tag,tag_id_hex=taghex,fields=collection_fields(data)))
 w,h=mesh['width'],mesh['height'];off=mesh['sections']['mesh']['offset'];cells=raw[off:off+w*h*12];native=bytearray(len(cells))
 for i in range(0,len(cells),12):
  for j in range(0,12,2):native[i+j:i+j+2]=cells[i+j:i+j+2][::-1]
 (scene/'terrain-cells.bin').write_bytes(native);heights=b''.join(native[i:i+2] for i in range(0,len(native),12));(scene/'terrain-heights.i16le').write_bytes(heights)
 (scene/'landscape-l1.collection').write_bytes(raw_tag(tags,'.256',mesh['header']['landscape_collection_tag']))
 axial=[]
 for (kind,tag),data in deps.items():
  if kind=='proj' and not u(data,0)&(1<<21):
   col=canonical.get(data[4:8].hex());seq=struct.unpack_from('>h',data,8)[0]
   if col is not None and seq>=0:axial.append([col,seq])
 (scene/'scene.json').write_text(json.dumps(dict(assets=assets,terrain=dict(width=w,height=h),axial_candidates=axial)),encoding='utf8')
 env=dict(os.environ,MYTH_LEVEL_BUILD=str(work),MYTH_MAP_TAG=mapid)
 for script in ('prepare_vr_assets','prepare_vr_models','prepare_animated_models','prepare_surface_assets','prepare_particle_assets','prepare_reflection_assets'):
  progress('変換: '+script,item=name)
  with (report/(script+'.log')).open('w',encoding='utf8') as logfile:
   r=subprocess.Popen([sys.executable,'-u',str(ROOT/'tools'/(script+'.py'))],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf8',errors='replace')
   tail=''
   for line in r.stdout:
    logfile.write(line);logfile.flush();print(line,end='',flush=True);tail=(tail+line)[-2000:]
   if r.wait():raise RuntimeError(script+': '+tail)
  print(mapid,script,'OK',flush=True)
 origin=[w/2,h/2,0]
 sky=Path(os.environ.get('MYTH_SKYBOX_ROOT',str(ROOT/'Skyboxes')))/(name+'.skybox.json');sky_yaw=json.loads(sky.read_text(encoding='utf8')).get('runtime_yaw_degrees',0) if sky.is_file() else 0
 if tagged:
  (out/'collections.bin').write_bytes(b'MVRCL002'+struct.pack('<I',len(assets))+b''.join(struct.pack('<8I',a['runtime_index'],*a['fields'],int(a['tag_id_hex'],16)) for a in assets))
 else:
  (out/'collections.bin').write_bytes(struct.pack('<I',len(assets))+b''.join(struct.pack('<7I',a['runtime_index'],*a['fields']) for a in assets))
 target=Path(os.environ.get("MYTH_OUTPUT_ASSETS",str(ROOT/"build/vr/assets")))/"levels"/key;target.mkdir(parents=True,exist_ok=True)
 for n in ('terrain-base.pack','terrain-base.png','connectors.pack','scene.pack','models.pack','sprite-indices.pack','animated-models.pack','surface-effects.pack','particles.pack','reflection-water.pack','collections.bin'):(shutil.move if os.environ.get("MYTH_COMPACT_BUILD")=="1" else shutil.copyfile)(out/n,target/n)
 from prepare_minimap_assets import build_minimap
 progress('ミニマップを作成',item=name)
 build_minimap(tags,mapid,target)
 result=dict(map_id=mapid,key=key,name=name,width=w,height=h,origin=origin,collections=len(assets),height_sha256=hashlib.sha256(heights).hexdigest(),global_tint_fraction=mesh['header']['global_tint_fraction'])
 (work/'manifest.json').write_text(json.dumps(result,indent=2),encoding='utf8')
 return '\t'.join(map(str,[key,int.from_bytes(mesh['header']['landscape_collection_tag'].encode('latin1'),'big'),w,h,*origin,sky_yaw,name,identity(raw)]))
if __name__=='__main__':
 import argparse
 from concurrent.futures import ThreadPoolExecutor,as_completed
 parser=argparse.ArgumentParser();parser.add_argument('--maps',nargs='*');parser.add_argument('--resume',action='store_true');parser.add_argument('--jobs',type=int,default=2);args=parser.parse_args()
 tags=inventory();old=json.loads((ROOT/'build/scene-bridge/scene.json').read_text());seed={a['tag_id_hex']:a['runtime_index'] for a in old['assets']}
 levels=campaign(tags)
 def one(level):
  m,n=level;key=m.encode('latin1').hex();row=ROOT/'build/levels'/key/'catalog-row-v2.txt'
  if args.resume and row.is_file() and (Path(os.environ.get('MYTH_OUTPUT_ASSETS',str(ROOT/'build/vr/assets')))/'levels'/key/'terrain-base.pack').is_file():
   from prepare_minimap_assets import build_minimap
   target=Path(os.environ.get("MYTH_OUTPUT_ASSETS",str(ROOT/"build/vr/assets")))/"levels"/key;target.mkdir(parents=True,exist_ok=True)
   if not (target/'minimap.rgba').is_file():build_minimap(tags,m,target)
   return key,row.read_text(encoding='utf8').strip()
  line=build(m,n,dict(seed),tags);row.write_text(line,encoding='utf8');return key,line
 rows={};errors=[]
 with ThreadPoolExecutor(max_workers=max(1,min(4,args.jobs))) as pool:
  work={pool.submit(one,level):level for level in levels if not args.maps or level[0].encode('latin1').hex() in args.maps}
  for future in as_completed(work):
   try:key,line=future.result();rows[key]=line;print('COMPLETE',key,flush=True)
   except Exception as e:errors.append((work[future],str(e)));print('FAILED',ascii(work[future]),str(e),flush=True)
 if errors:raise RuntimeError(errors)
 target=ROOT/'build/vr/assets/levels';target.mkdir(parents=True,exist_ok=True)
 for m,n in levels:
  key=m.encode('latin1').hex();row=ROOT/'build/levels'/key/'catalog-row-v2.txt'
  if key not in rows and row.is_file():rows[key]=row.read_text(encoding='utf8').strip()
 lines=[rows[m.encode('latin1').hex()] for m,n in levels if m.encode('latin1').hex() in rows]
 temporary=target/'catalog.tsv.tmp';temporary.write_text('\n'.join(lines)+'\n',encoding='utf8');temporary.replace(target/'catalog.tsv')
 print('Built campaign maps:',len(lines),flush=True)

if __name__=="__main__":
 rows=[]
 for p in sorted((ROOT/"build/levels").glob("*/decoded-map.json")):
  m=json.loads(p.read_text(encoding="utf8"));rows.append(p.parent.name+"\t"+m["header"]["landscape_collection_tag"].encode("latin1").hex())
 (ROOT/"build/vr/assets/levels/detail-bindings.tsv").write_text("\n".join(rows)+"\n")
