import json,struct,sys,zlib,os
from pathlib import Path
sys.path.insert(0,'tools')
from tag_sources import inventory
T=inventory();ROOT=Path('.')
def raw(k,t):
 p,r=T[k,t]
 with Path(p).open('rb') as f:f.seek(r['offset']);return f.read(r['size'])
def u(b,p):return struct.unpack_from('>I',b,p)[0]
def refs(b,kind,start,tag):
 d=u(b,248);result=[]
 hueNames=[b[d+u(b,84)+i*128:d+u(b,84)+i*128+64].split(b"\0")[0].decode("macroman").lower() for i in range(u(b,80))]
 for i in range(u(b,96)):
  r=d+u(b,100)+i*128;p=d+u(b,r+64);w,h=struct.unpack_from('>HH',b,p)
  flags=struct.unpack_from('>H',b,p+6)[0]; skip=52+(0 if flags&32 else 4*(h-1)); size=u(b,r+68)
  crc=zlib.crc32(b[p+skip:p+size])^0xffffffff
  result.append([kind,start+i,tag,str(crc),w,h,hueNames])
 return result
for work in Path(os.environ.get('MYTH_LEVELS_ROOT','build/levels')).iterdir():
 if not (work/'scene/scene.json').exists():continue
 scene=json.loads((work/'scene/scene.json').read_text());rows=[];n=1
 for a in scene['assets']:
  b=(work/'scene'/a['file']).read_bytes();rows+=refs(b,0,n,a['tag_id_hex']);n+=u(b,96)
 n=0
 for a in scene['assets']:
  if a.get('model_material',False):continue
  b=(work/'scene'/a['file']).read_bytes();rows+=refs(b,3,n,a['tag_id_hex']);n+=u(b,96)
 models=json.loads((work/'reports/vr-models.json').read_text());cols=set();n=0;animtags=set()
 for m in models:
  b=raw('mode',m['model']);g=raw('geom',b[4:8].decode('macroman'));core=raw('core',g[4:8].decode('macroman'));col=core[:4].decode('macroman')
  if col not in cols:
   cols.add(col);b=raw('.256',col);rows+=refs(b,1,n,core[:4].hex());n+=u(b,96)
  if m['animated_first_pose_only']:
   b=raw('anim',m['tag']);count=struct.unpack_from('>H',b,4)[0]
   animtags.update(b[12+i*16:16+i*16].decode('macroman') for i in range(count))
 cols=set();geoms=set();n=0
 for tag in sorted(animtags):
  b=raw('mode',tag);geom=b[4:8]
  if geom in geoms:continue
  geoms.add(geom);g=raw('geom',geom.decode('macroman'));core=raw('core',g[4:8].decode('macroman'));col=core[:4].decode('macroman')
  if col not in cols:
   cols.add(col);b=raw('.256',col);rows+=refs(b,2,n,core[:4].hex());n+=u(b,96)
 header=json.loads((work/'decoded-map.json').read_text(encoding='utf8'))['header'];tint=struct.unpack('>3H',header['global_tint_color'].encode('latin1')[:6]);((Path(os.environ.get('MYTH_OUTPUT_ASSETS','build/vr/assets'))/'levels')/work.name/'hd-tint.bin').write_bytes(struct.pack('<4I',header['global_tint_fraction'],*tint))
 target=(Path(os.environ.get('MYTH_OUTPUT_ASSETS','build/vr/assets'))/'levels')/work.name/'hd-bindings.json';target.write_text(json.dumps(rows,ensure_ascii=True,separators=(',',':')),encoding='utf8')
 print(work.name,len(rows))
