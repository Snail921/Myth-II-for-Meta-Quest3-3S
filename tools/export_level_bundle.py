"""Export version-1 external VR level bundles from a built APK (no conversion)."""
import argparse,hashlib,json,zipfile,re,os,shutil
from contextlib import nullcontext
from pathlib import Path
FILES=['collections.bin','scene.pack','sprite-indices.pack','models.pack','animated-models.pack','connectors.pack','particles.pack','reflection-water.pack','surface-effects.pack','minimap.rgba','hd-bindings.json','hd-tint.bin','detail-bindings.tsv','fog.pack','water-light.pack']
def export(apk,dest,keys,assets=None):
 dest=Path(dest)
 if dest.exists():raise ValueError('Output already exists; use a new directory to avoid stale files')
 if not all(re.fullmatch('[0-9a-f]{8}',k) for k in keys):raise ValueError('Level keys must be eight lowercase hex digits')
 with (nullcontext(None) if assets else zipfile.ZipFile(apk)) as z:
  def read(path):return (Path(assets)/path.removeprefix('assets/')).read_bytes() if assets else z.read(path)
  catalog=read('assets/levels/catalog.tsv').decode().splitlines();selected=[r for r in catalog if r.split('\t')[0] in keys];assert len(selected)==len(set(keys)), 'Unknown or duplicate level key'
  for line in selected:
   key=line.split('\t')[0];folder=dest/'levels'/key;folder.mkdir(parents=True);spec={}
   for name in FILES:
    path='assets/'+name if name in ('fog.pack','water-light.pack') else 'assets/levels/'+name if name=='detail-bindings.tsv' else 'assets/levels/'+key+'/'+name
    b=read(path)
    if name=='detail-bindings.tsv':b=(''.join(r+'\n' for r in b.decode().splitlines() if r.split('\t')[0]==key)).encode()
    
    if assets and os.environ.get('MYTH_COMPACT_BUILD')=='1' and name not in ('fog.pack','water-light.pack','detail-bindings.tsv'):shutil.move(str(Path(assets)/path.removeprefix('assets/')),folder/name)
    else:(folder/name).write_bytes(b)
    spec[name]={'size':len(b),'sha256':hashlib.sha256(b).hexdigest()}
   (folder/'manifest.json').write_text(json.dumps({'version':1,'catalog':line,'files':spec},indent=2),encoding='utf8')
  (dest/'catalog.tsv').write_text('\n'.join(selected)+'\n',encoding='utf8')
 print(json.dumps({'directory':str(dest),'levels':keys,'bytes':sum(p.stat().st_size for p in dest.rglob('*') if p.is_file())}))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--apk',default='build/myth-standalone-185-candidate.apk');p.add_argument('--assets-dir',help='Use an existing converted asset directory instead of the APK');p.add_argument('--out',required=True);p.add_argument('--levels',nargs='+',required=True);a=p.parse_args();export(a.apk,a.out,a.levels,a.assets_dir)
