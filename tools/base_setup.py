"""Build user-owned base game and VR bundles. Never executes the updater."""
import argparse,json,os,subprocess,sys,shutil,hashlib
from pathlib import Path
from plugin_manager_backend import ROOT,digest,Device
from plugin_sources import profile,archive
from build_progress import progress
ENGINE='aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c'
SHARED=('info-font.rgba','info-decoration.pack','dialog-templates.pack')
def extract_entry(seven,source,name,target):
 # stdout extraction: archive paths never become filesystem destinations.
 target=Path(target);target.parent.mkdir(parents=True,exist_ok=True)
 with target.open('wb') as f:
  result=subprocess.run([str(seven),'x','-so','-y',str(source),name],stdout=f,stderr=subprocess.PIPE)
 if result.returncode or not target.stat().st_size:raise ValueError('Archive entry could not be read: '+name)
def prepare_game(iso,updater,dest,seven):
 dest=Path(dest)
 if dest.exists():raise ValueError('Choose a new output folder')
 dest.mkdir(parents=True)
 expected=json.loads((ROOT/'tools/base-game-files.json').read_text(encoding='utf8'))
 for i,item in enumerate(expected):
  path=item['path'];progress('Extracting original files',i,len(expected),path)
  extract_entry(seven,iso if path.startswith('tags/') else updater,path.replace('/','\\'),dest/path)
  if digest(dest/path)!=item['sha256']:raise ValueError('Unsupported original archive: '+path)
 for item in json.loads((ROOT/'tools/base-runtime-files.json').read_text(encoding='utf8')):
  name=item['path'];extract_entry(seven,updater,name.replace('/','\\'),dest/name)
  if digest(dest/name)!=item['sha256']:raise ValueError('Unsupported runtime file: '+name)
 if digest(dest/'Myth II.exe')!=ENGINE:raise ValueError('Unsupported game executable')
 for name in ('01','03a','04','09','14','17','20','25','epilogue','prologue'):
  progress('Extracting cutscenes',item=name);extract_entry(seven,iso,'cutscenes\\'+name+'.smk',dest/'cutscenes'/(name+'.smk'))
 return profile([],dest)
def add_addons(game,sources):
 import time
 for source in sources:
  source=Path(source);a=archive(source)
  if a['kind']!=6:raise ValueError('Choose a high-resolution ADDON archive: '+str(source))
  target=Path(game)/'plugins'/source.name
  if target.exists():raise ValueError('Duplicate optional addon name: '+source.name)
  done=0;last=0
  with source.open('rb') as src,target.open('xb') as dst:
   while True:
    b=src.read(1024*1024)
    if not b:break
    dst.write(b);done+=len(b);now=time.monotonic()
    if now-last>.5:progress('Copying HD addon',done//1048576,(a['size']+1048575)//1048576,source.name+' (MiB)');last=now
  if digest(target)!=a['sha256']:raise ValueError('HD addon changed during copy')
  progress('Copying HD addon',done//1048576,(a['size']+1048575)//1048576,source.name+' (MiB)')

def validate_visuals(logo=None,cd_face=None):
 from PIL import Image
 for source in (logo,cd_face):
  if not source:continue
  if not Path(source).is_file():raise ValueError('Image file is missing: '+str(source))
  with Image.open(source) as image:
   if image.format!='PNG':raise ValueError('Select a PNG image: '+str(source))
   image.verify()

def run(iso,updater,out,seven,hd=(),maps=None,skyboxes=None,logo=None,cd_face=None):
 validate_visuals(logo,cd_face)
 out=Path(out).resolve()
 for source in (iso,updater,seven):
  if not Path(source).is_file():raise ValueError('Input file is missing: '+str(source))
 probe=out.parent
 while not probe.exists():probe=probe.parent
 needed=8*1024**3+sum(Path(source).stat().st_size for source in hd)
 if shutil.disk_usage(probe).free<needed:raise ValueError('At least 8 GiB plus optional addon sizes of free PC storage is required')
 out.mkdir(parents=True,exist_ok=False)
 resolved=prepare_game(iso,updater,out/'Game',seven)
 add_addons(out/'Game',hd)
 cmd=[sys.executable,'-u',str(ROOT/'tools/convert_plugin_levels.py'),'--game-root',str(out/'Game'),'--out',str(out/'conversion')]
 if maps:cmd+=['--maps',*maps]
 subprocess.run(cmd,check=True,cwd=ROOT,env=dict(os.environ,MYTH_COMPACT_BUILD="1",PYTHONIOENCODING="utf-8",MYTH_SKYBOX_ROOT=str(Path(skyboxes).resolve()) if skyboxes else ""))
 return finish(out,out/'conversion',skyboxes,logo,cd_face)

def finish(out,conversion,skyboxes=None,logo=None,cd_face=None):
 validate_visuals(logo,cd_face)
 out=Path(out);conversion=Path(conversion)
 shared=out/'SharedAssets';env=dict(os.environ,MYTH_TAG_PROFILE=str(conversion/'tag-profile.json'),MYTH_SHARED_ASSETS=str(shared))
 subprocess.run([sys.executable,'-u',str(ROOT/'tools/prepare_info_font.py')],cwd=ROOT,env=env,check=True)
 (shared/'manifest.json').write_text(json.dumps(dict(version=1,files={n:dict(size=(shared/n).stat().st_size,sha256=digest(shared/n)) for n in SHARED}),indent=2),encoding='utf8')
 # Publish only the verified final bundle; keep work logs outside the deployment tree.
 (out/'LevelPacks').mkdir(exist_ok=True);shutil.move(str(conversion/'bundle'),out/'LevelPacks/soulblighter')
 for source,name in ((logo,'Logo.png'),(cd_face,'CD_Face.png')):
  if source:shutil.copyfile(source,shared/name)
 if skyboxes:
  sky=out/'Skyboxes';sky.mkdir(exist_ok=True)
  from PIL import Image
  for source in Path(skyboxes).iterdir():
   if source.is_file() and source.suffix.lower() in ('.png','.jpg','.jpeg'):
    with Image.open(source) as im:
     if im.width!=im.height*2 or im.height>2048:raise ValueError('Skybox must be 2:1, at most 4096x2048: '+source.name)
    shutil.copyfile(source,sky/source.name)
 manifest={p.relative_to(out).as_posix():dict(size=p.stat().st_size,sha256=digest(p)) for folder in ('Game','SharedAssets','LevelPacks','Skyboxes') for p in (out/folder).rglob('*') if p.is_file()}
 (out/'setup-manifest.json').write_text(json.dumps(dict(version=1,files=manifest),indent=2),encoding='utf8')
 progress('Setup data ready',1,1,str(out));return out
if __name__=='__main__':
 os.environ['MYTH_BUILD_PROGRESS']='1'
 p=argparse.ArgumentParser();p.add_argument('--iso',required=True);p.add_argument('--updater',required=True);p.add_argument('--out',required=True);p.add_argument('--seven-zip',default='C:/Program Files/7-Zip/7z.exe');p.add_argument('--hd',action='append',default=[]);p.add_argument('--maps',nargs='+');p.add_argument('--skyboxes');p.add_argument('--logo');p.add_argument('--cd-face');a=p.parse_args()
 run(a.iso,a.updater,a.out,a.seven_zip,a.hd,a.maps,a.skyboxes,a.logo,a.cd_face)
