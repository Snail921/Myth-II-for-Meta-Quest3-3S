"""Bounded Myth II archive index; explicit plugin order and isolated resolver profile."""
import hashlib,json,struct,re
from pathlib import Path
from build_progress import progress
ROOT=Path(__file__).resolve().parents[1]
def archive(path):
 path=Path(path).resolve();progress('アーカイブを検査',item=path.name);size=path.stat().st_size
 with path.open('rb') as f:
  h=f.read(128)
  if len(h)!=128 or h[124:128]!=b'dng2':raise ValueError('Not a Myth II archive: '+str(path))
  kind,version=struct.unpack_from('>HH',h);ne,nt=struct.unpack_from('>HH',h,100);start=128+ne*112;end=start+nt*64
  if kind>7 or end>size:raise ValueError('Invalid archive table: '+str(path))
  entries=[]
  for _ in range(ne):
   r=f.read(112);decode=lambda b:b.split(b'\0')[0].decode('mac_roman');entries.append({'id':decode(r[:16]),'order':decode(r[16:48]),'name':decode(r[48:112])})
  tags=[];seen=set()
  for _ in range(nt):
   r=f.read(64);off,n=struct.unpack_from('>II',r,44)
   if r[60:64]!=b'mth2' or off<end or off+n>size:raise ValueError('Invalid tag bounds: '+str(path))
   typ=r[36:40].decode('mac_roman');key=r[40:44].decode('mac_roman')
   if (typ,key) in seen:raise ValueError('Duplicate tag in archive: '+repr((typ,key)))
   seen.add((typ,key));tags.append(dict(type=typ,id=key,id_hex=r[40:44].hex(),type_hex=r[36:40].hex(),name=r[4:36].split(b'\0')[0].decode('mac_roman'),offset=off,size=n,version=struct.unpack_from('>H',r,56)[0]))
 with path.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
 return dict(path=str(path),size=size,sha256=digest,kind=kind,version=version,entries=entries,tags=tags)
def profile(plugins, game_root=None):
 if game_root is not None:
  game=Path(game_root).resolve()
  exe=game/'Myth II.exe'
  if not exe.is_file() or hashlib.sha256(exe.read_bytes()).hexdigest()!='aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c':
   raise ValueError('対応するMyth II 1.8.5のMyth II.exeがありません。')
  if (game/'local').exists() and any(p.is_file() and p.stat().st_size for p in (game/'local').rglob('*')):
   raise ValueError('localフォルダの上書き素材は未対応です。local素材を使わない原作フォルダを指定してください。')
  expected=json.loads((ROOT/'tools/base-game-files.json').read_text(encoding='utf8'));found=[];paths=set()
  for row in expected:
   p=game/row['path']
   a=archive(p)
   if a['sha256']!=row['sha256']:raise ValueError('対応確認済みの原作素材と一致しません: '+str(p))
   found.append(a);paths.add(p.resolve())
  for directory in ('tags','plugins'):
   for p in (game/directory).iterdir():
    if not p.is_file() or p.resolve() in paths:continue
    with p.open('rb') as f:h=f.read(128)
    if len(h)==128 and h[124:]==b'dng2' and struct.unpack_from('>H',h)[0] in (2,3):
     raise ValueError('追加の基礎/PATCHアーカイブは未対応です: '+str(p))
  selected=[archive(p) for p in plugins]
  for a in selected:
   if a['kind']!=1:raise ValueError('Select a gameplay PLUGIN archive')
   if a['version'] not in (0,65535) and not (a['version']==2 and len(selected)==1):raise ValueError('Unsupported plugin dependency flags')
  if len({a['path'] for a in selected})!=len(selected):raise ValueError('Duplicate selected plugin')
  return resolve(found+selected,[a['path'] for a in selected])

 old=json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf8'))
 base=[archive(ROOT/a['path']) for a in old if a['archive_type']=='FOUNDATION']
 patches=[]
 for p in (ROOT/'research/extracted/update-1.8.5/plugins').iterdir():
  if not p.is_file():continue
  a=archive(p)
  if a['kind']==2:patches.append(a)
 patches.sort(key=lambda a:tuple(int(x) for x in re.findall(r'\d+',Path(a['path']).name)))
 selected=[]
 for p in plugins:
  a=archive(p)
  if a['kind']!=1:raise ValueError('Select a gameplay PLUGIN archive (HD ADDONs are loaded separately): '+str(p))
  # Bit 1 advertises optional vTFL compatibility, not a mandatory dependency.
  # A single archive in normal Myth II mode has no inter-plugin ordering ambiguity.
  # Unity/vTFL and multi-plugin flag combinations remain unsupported.
  if a['version'] not in (0,65535) and not (a['version']==2 and len(plugins)==1):raise ValueError('Plugin dependency/load-order flags require a separate resolver profile; refusing implicit order: '+str(p))
  selected.append(a)
 if len({a['path'] for a in selected})!=len(selected):raise ValueError('Duplicate selected plugin')
 return resolve(base+patches+selected,[a['path'] for a in selected])

def resolve(archives,selected):
 resolved={};overrides=[]
 for a in archives:
  for t in a['tags']:
   key=t['type'],t['id']
   if key in resolved and a['path'] in selected:overrides.append({'type':key[0],'id_hex':t['id_hex'],'from':resolved[key][0],'to':a['path']})
   resolved[key]=(a['path'],t)
 return {'version':1,'engine':'1.8.5','order':'foundation, official patches, explicitly selected plugins in argument order (last wins)','archives':[{k:v for k,v in a.items() if k!='tags'} for a in archives],'selected':selected,'tags':[[k[0],k[1],v[0],v[1]] for k,v in sorted(resolved.items())],'overrides':overrides}
