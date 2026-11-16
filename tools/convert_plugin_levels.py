"""Resolve an explicitly selected plugin set and convert maps to an external VR bundle.
The original plugins must also be enabled in the engine with the same load order.
"""
import argparse,json,os,sys,struct,math,subprocess,hashlib
from pathlib import Path
from plugin_sources import ROOT,profile
from build_progress import progress,tracked

def main():
 os.environ['MYTH_BUILD_PROGRESS']='1'
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--game-root',help='Original 1.8.5 game directory');p.add_argument('--plugin',action='append',default=[],help='Gameplay archive; repeat in override order (last wins)');p.add_argument('--out',required=True);p.add_argument('--maps',nargs='+',help='Eight-digit hex mesh IDs; default all selected plugin entrypoints');p.add_argument('--resume',action='store_true');p.add_argument('--list',action='store_true',help='Write entrypoint/dependency preflight report without generating assets');a=p.parse_args()
 dest=Path(a.out).resolve()
 if dest.exists() and not a.resume:raise ValueError('Use a new output directory to avoid mixing builds')
 progress('原作・プラグインの照合');dest.mkdir(parents=True,exist_ok=a.resume);resolved=profile(a.plugin,a.game_root);profile_path=dest/'tag-profile.json'
 if a.resume and json.loads(profile_path.read_text(encoding='utf8'))!=resolved:raise ValueError('Resume source profile changed')
 profile_path.write_text(json.dumps(resolved,ensure_ascii=True,indent=2),encoding='utf8')
 os.environ['MYTH_TAG_PROFILE']=str(profile_path);os.environ['MYTH_LEVELS_ROOT']=str(dest/'work');os.environ['MYTH_OUTPUT_ASSETS']=str(dest/'assets')
 from tag_sources import inventory
 from level_assets import raw_tag,parse_mesh,schemas
 from build_level_assets import build
 progress('素材一覧の読み込み');tags=inventory();names={}
 for archive in resolved['archives']:
  if archive['path'] in resolved['selected'] or (not resolved['selected'] and archive['kind']==3):
   for entry in archive['entries']:names[entry['id']]=entry
 if not resolved['selected'] and ('mesh','00tm') in tags:names['00tm']=dict(id='00tm',name='Tutorial',order='00')
 levels=[];excluded=[]
 for entry in tracked(sorted(names.values(),key=lambda e:e['order']),'レベル情報の検査'):
  tag=entry['id']
  if len(tag)!=4 or not tag.isascii():raise ValueError('Non-ASCII mesh IDs require additional codec support: '+repr(tag))
  if ('mesh',tag) not in tags:raise ValueError('Missing entrypoint mesh '+repr(tag))
  raw=raw_tag(tags,'mesh',tag);m=parse_mesh(raw,schemas())
  if not m['header']['flags']&65536 and tag!='00tm':continue
  if not m['header']['flags']&0x100000 and tag!='00tm':
   excluded.append(dict(key=tag.encode('ascii').hex(),name=entry['name'],reason='mesh IS_COMPLETE flag is clear'));continue
  if m['header']['flags']&(0x40000000|0x80000000):raise ValueError('Mesh requires vTFL or another plugin: '+repr(tag))
  name=entry['name'];key=tag.encode('ascii').hex()
  if not name or len(name.encode('utf8'))>=64 or any(c in name for c in '\t\r\n\0'):raise ValueError('Unsupported level display name')
  levels.append((key,tag,name))
 if not levels:raise ValueError('No single-player mesh entrypoints')
 (dest/'excluded-entrypoints.json').write_text(json.dumps(excluded,ensure_ascii=True,indent=2),encoding='utf8')
 print(json.dumps({'levels':[{'key':k,'name':n} for k,t,n in levels],'override_count':len(resolved['overrides'])},ensure_ascii=True),flush=True)
 if a.maps:
  unknown=set(a.maps)-{k for k,t,n in levels}
  if unknown:raise ValueError('Unknown map keys '+repr(unknown))
  levels=[x for x in levels if x[0] in a.maps]
 from plugin_preflight import inspect
 checks={}
 for index,(k,t,n) in enumerate(levels,1):
  progress('レベルの依存素材検査',index-1,len(levels),n)
  checks[k]=dict(name=n,**inspect(tags,t))
  progress('レベルの依存素材検査',index,len(levels),n)
 (dest/'preflight.json').write_text(json.dumps(checks,ensure_ascii=True,indent=2),encoding='utf8')
 if a.list:
  print(json.dumps(checks,ensure_ascii=True),flush=True);return
 rejected=[k for k,v in checks.items() if not v['supported']]
 if rejected:raise ValueError('Runtime cannot safely bind these maps: '+', '.join(rejected)+'; see '+str(dest/'preflight.json'))
 rows=[];fog=[];lights=[];detail=[]
 for key,tag,name in levels:
  progress('レベル変換',len(rows),len(levels),name);print('CONVERT',key,name,flush=True);checkpoint=dest/'work'/key/'catalog-row.txt'
  if a.resume and checkpoint.is_file():rows.append(checkpoint.read_text(encoding='utf8'))
  else:
   rows.append(build(tag,name,{},tags));checkpoint.write_text(rows[-1],encoding='utf8')
  raw=raw_tag(tags,'mesh',tag);m=parse_mesh(raw,schemas());rgb=struct.unpack_from('>3H',raw,496);density=struct.unpack_from('>f',raw,504)[0];fog.append(tag.encode('ascii')+struct.pack('<4f',*(v/65535 for v in rgb),density))
  detail.append(key+'\t'+m['header']['landscape_collection_tag'].encode('latin1').hex())
  lighting=m['header']['mesh_lighting_tag']
  if ('meli',lighting) in tags:
   i,j,k=struct.unpack_from('>3i',raw_tag(tags,'meli',lighting));v=(i/65536.,-k/65536.,-j/65536.);length=math.sqrt(sum(x*x for x in v))
   if length>0 and v[1]>0:lights.append(tag.encode('ascii')+struct.pack('<3f',*(x/length for x in v)))
 assets=dest/'assets';(assets/'levels/catalog.tsv').write_text('\n'.join(rows)+'\n',encoding='utf8');(assets/'levels/detail-bindings.tsv').write_text('\n'.join(detail)+'\n',encoding='utf8')
 (assets/'fog.pack').write_bytes(struct.pack('<I',len(fog))+b''.join(fog));(assets/'water-light.pack').write_bytes(struct.pack('<I',len(lights))+b''.join(lights))
 progress('高解像度素材の対応表を作成')
 subprocess.run([sys.executable,'-u',str(ROOT/'tools/build_hd_bindings.py')],cwd=ROOT,env=os.environ,check=True)
 # Do not publish a bundle compiled against a mixture of changing archives.
 for archive in tracked(resolved['archives'],'原作ファイルの再検証'):
  with open(archive['path'],'rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
  if digest!=archive['sha256']:raise ValueError('Source changed during conversion: '+archive['path'])
 from export_level_bundle import export
 progress('外部パックを書き出し')
 export(None,dest/'bundle',[k for k,t,n in levels],assets)
 (dest/'bundle/source-profile.json').write_text(json.dumps({k:v for k,v in resolved.items() if k!='tags'},ensure_ascii=True,indent=2),encoding='utf8')
 from bind_bundle_sources import bind
 progress('原作素材との対応を記録')
 bind(dest/'bundle')
 from verify_level_bundle import verify
 progress('完成パックを検証')
 verified=verify(dest/'bundle')
 (dest/'bundle-verification.json').write_text(json.dumps(verified,indent=2),encoding='utf8')
 print('READY',dest/'bundle',flush=True)
if __name__=='__main__':main()
