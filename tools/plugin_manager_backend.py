"""PC plugin workflow. No APK changes; single gameplay plugin only."""
import hashlib,json,os,re,shlex,subprocess,sys,tarfile,uuid
from pathlib import Path
from plugin_sources import ROOT,archive
from verify_level_bundle import verify
PACKAGE='org.mythvr.standalone'
REMOTE='/sdcard/Android/data/'+PACKAGE+'/files/LevelPacks'
GAME='/sdcard/Android/data/'+PACKAGE+'/files/Game'
ADB_DEFAULT=Path(os.environ.get('LOCALAPPDATA',''))/'Android/Sdk/platform-tools/adb.exe'
def digest(path):
 with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def run_conversion(game,plugin,output,list_only=False,log=print):
 if not list_only:log('基本色画像とユニファイド影用データも生成します / Exporting unlit terrain colors for unified shadows')
 output=Path(output).resolve();cmd=[sys.executable,'-u',str(ROOT/'tools/convert_plugin_levels.py'),'--game-root',str(game),'--out',str(output)]
 if plugin:cmd.extend(['--plugin',str(plugin)])
 if list_only:cmd.append('--list')
 output.parent.mkdir(parents=True,exist_ok=True)
 env=os.environ.copy();env['PYTHONIOENCODING']='utf-8'
 with output.with_suffix('.log').open('w',encoding='utf8') as report:
  p=subprocess.Popen(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf8',errors='replace',env=env)
  for line in p.stdout:
   report.write(line);report.flush();log(line.rstrip())
  if p.wait():raise ValueError('検査/変換に失敗しました。ログ: '+str(output.with_suffix('.log')))
 return output if list_only else output/'bundle'
def bundle_inputs(bundle,plugin):
 bundle=Path(bundle).resolve();plugin=Path(plugin).resolve() if plugin else None;result=verify(bundle)
 if any(p.is_symlink() for p in bundle.rglob('*')):raise ValueError('シンボリックリンクを含むパックは配置できません。')
 req=json.loads((bundle/'source-files.json').read_text(encoding='utf8'))
 selected=req.get('active_plugins',[])
 if len(selected)>1:raise ValueError('配置ツールは単独ゲームプレイプラグインのみ対応しています。')
 if bool(selected)!=bool(plugin):raise ValueError('原作パックはプラグイン欄を空に、プラグインパックは対応するファイルを指定してください。')
 matches=[f for f in req['files'] if selected and f.get('runtime_identity')==selected[0]]
 if selected and len(matches)!=1:raise ValueError('原作プラグインの識別情報が不正です。')
 original=matches[0] if selected else None
 for item in req['files']:
  path=item['path']
  if len(path.split('/'))!=2 or path.split('/')[0] not in ('tags','plugins') or '\\' in path or '\0' in path or Path(path).name in ('.','..') or not re.fullmatch('[0-9a-f]{64}',item['sha256']):raise ValueError('不正な原作ファイル要件です。')
 if original is not None and (original['path']!='plugins/'+plugin.name or digest(plugin)!=original['sha256']):raise ValueError('選択したプラグインが変換時のものと一致しません。')
 return result,req,original
class Device:
 def __init__(self,adb=ADB_DEFAULT,serial='',log=print):self.adb=str(adb);self.serial=serial;self.log=log
 def call(self,*args,timeout=None):
  cmd=[self.adb]+(['-s',self.serial] if self.serial else [])+list(args)
  p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,encoding='utf8',errors='replace',timeout=timeout)
  if p.returncode:raise RuntimeError(p.stdout.strip() or 'ADB failed')
  return p.stdout.strip()
 def shell(self,command):return self.call('shell',command)
 def devices(self):return [r.split()[0] for r in self.call('devices').splitlines()[1:] if len(r.split())==2 and r.split()[1]=='device']
 def ready(self):
  ids=self.devices()
  if not self.serial:
   if len(ids)!=1:raise ValueError('接続済みのQuestを1台選択してください。USBデバッグの許可も確認してください。')
   self.serial=ids[0]
  if self.serial not in ids:raise ValueError('選択したQuestに接続できません。')
  if 'package:' not in self.shell('pm path '+PACKAGE):raise ValueError('対応APKがインストールされていません。')
  if re.search(r'(?i)myth ii\.exe|wineserver|box64',self.shell('ps -A -o ARGS')):raise ValueError('原作ゲームを終了してから配置してください。プレイを自動終了はしません。')
 def exists(self,p):return self.shell('if test -e '+shlex.quote(p)+'; then echo yes; fi')=='yes'
 def sha(self,p):return self.shell('sha256sum '+shlex.quote(p)).split()[0]
 def move(self,a,b):
  if self.exists(b):raise ValueError('退避先が既に存在します: '+b)
  self.shell('mv '+shlex.quote(a)+' '+shlex.quote(b))
 def deploy(self,bundle,plugin,logdir):
  bundle=Path(bundle).resolve();plugin=Path(plugin).resolve() if plugin else None;result,req,original=bundle_inputs(bundle,plugin);self.ready()
  game=GAME if self.exists(GAME+'/Myth II.exe') else '/sdcard/Download/MythII-Standalone185'
  if self.sha(game+'/Myth II.exe')!=req['engine_sha256']:raise ValueError('Questの原作EXEが変換対象と異なります。')
  for item in req['files']:
   if item is original:continue
   if self.sha(game+'/'+item['path'])!=item['sha256']:raise ValueError('Questの原作素材が一致しません: '+item['path'])
  token=uuid.uuid4().hex;key=hashlib.sha256(original['path'].casefold().encode()).hexdigest()[:16] if original else 'base'
  target=REMOTE+('/plugin-'+key if original else '/soulblighter');staging=REMOTE+'/.install-'+token;temp='/data/local/tmp/plugin-'+token
  logdir=Path(logdir);logdir.mkdir(parents=True,exist_ok=True);tar=logdir/(token+'.tar');journal=logdir/(token+'.json')
  files=sorted(p for p in bundle.rglob('*') if p.is_file());total=sum(p.stat().st_size for p in files)
  free=int(self.shell('df -k /sdcard').splitlines()[-1].split()[3])*1024
  if free<total*2+(plugin.stat().st_size*2 if plugin else 0)+256*1024**2:raise ValueError('Questの空き容量が不足しています（検証用の一時領域も必要です）。')
  # Find older packs for this same single gameplay archive; preserve other campaigns.
  old=[]
  names=self.shell('find '+shlex.quote(REMOTE)+' -mindepth 1 -maxdepth 1 -type d').splitlines()
  for path in names:
   if not path.startswith(REMOTE+'/') or '/' in path[len(REMOTE)+1:] or Path(path).name.startswith('.'):continue
   if not self.exists(path+'/source-files.json'):continue
   existing=json.loads(self.shell('cat '+shlex.quote(path+'/source-files.json')))
   active=existing.get('active_plugins',[])
   same=([f for f in existing.get('files',[]) if f.get('path','').casefold()==original['path'].casefold() and f.get('runtime_identity') in active] if original else not active)
   if same:
    if original and len(active)!=1:raise ValueError('同じプラグインを含む複合パックがあります。先に構成を整理してください: '+path)
    old.append(path)
  if self.exists(target) and target not in old:raise ValueError('配置先に別のパックがあります。上書きを中止しました。')
  changes=[];state={'status':'staging','bundle':str(bundle),'target':target,'backups':changes,'levels':len(result['levels'])}
  def save():journal.write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf8')
  save()
  try:
   self.log(f'Plugin “{plugin.name}” を転送しています…' if plugin else '原作レベルパックを転送しています…')
   with tarfile.open(tar,'w') as out:
    for p in files:out.add(p,arcname=p.relative_to(bundle).as_posix(),recursive=False)
   self.call('push',str(tar),temp+'.tar');self.shell('mkdir -p '+shlex.quote(staging));self.shell('tar -xf '+shlex.quote(temp+'.tar')+' -C '+shlex.quote(staging))
   for index,p in enumerate(files):
    if self.sha(staging+'/'+p.relative_to(bundle).as_posix())!=digest(p):raise ValueError('転送後の検証に失敗しました: '+p.name)
    if index%16==0:self.log('転送検証: '+str(index+1)+' / '+str(len(files)))
   remote_plugin=game+'/'+original['path'] if original else None;replace=bool(original) and (not self.exists(remote_plugin) or self.sha(remote_plugin)!=original['sha256'])
   if replace:
    self.call('push',str(plugin),temp+'.plugin')
    if self.sha(temp+'.plugin')!=original['sha256']:raise ValueError('原作プラグインの転送検証に失敗しました。')
   self.ready() # Do not publish while gameplay starts during a long transfer.
   self.call('shell','am','force-stop',PACKAGE) # Only the launcher remains; refresh its cached catalog.
   for path in old:
    backup=REMOTE+'/.'+Path(path).name+'.backup-'+token;self.move(path,backup);changes.append([path,backup]);save()
   if replace:
    if self.exists(remote_plugin):
     backup=game+'/.vr-plugin-backups/'+token+'/'+plugin.name;self.shell('mkdir -p '+shlex.quote(game+'/.vr-plugin-backups/'+token));self.move(remote_plugin,backup);changes.append([remote_plugin,backup]);save()
    self.move(temp+'.plugin',remote_plugin);state['new_plugin']=remote_plugin;save()
   self.move(staging,target);state['published']=True;save()
   state['status']='complete';save();self.log('配置完了: '+str(len(result['levels']))+('面。起動後、原作でプラグインを選択してください。' if plugin else '面。原作のマップ一覧から選択してください。'))
  except Exception:
   if state.get('published'):self.move(target,staging)
   if state.get('new_plugin') and self.exists(remote_plugin):self.move(remote_plugin,temp+'.failed-plugin')
   for dest,backup in reversed(changes):
    if not self.exists(dest):self.move(backup,dest)
   state['status']='failed_rolled_back';save();raise
  finally:
   # Exact temporary file paths only; never recursively remove device folders.
   self.shell('rm -f '+shlex.quote(temp+'.tar'));tar.unlink(missing_ok=True)
  try:self.call('shell','am','start','-n',PACKAGE+'/.StandaloneActivity')
  except Exception as e:self.log('配置は完了しました。アプリを手動で開いてください: '+str(e))
  return journal


def run_setup(iso,updater,seven,hd,dest,log=print,skyboxes=None,logo=None,cd_face=None):
 dest=Path(dest).resolve();dest.parent.mkdir(parents=True,exist_ok=True)
 cmd=[sys.executable,'-u',str(ROOT/'tools/base_setup.py'),'--iso',iso,'--updater',updater,'--seven-zip',seven,'--out',str(dest)]
 for addon in hd:cmd+=['--hd',addon]
 if skyboxes:cmd+=['--skyboxes',skyboxes]
 if logo:cmd+=['--logo',logo]
 if cd_face:cmd+=['--cd-face',cd_face]
 with dest.with_suffix('.log').open('w',encoding='utf8') as report:
  p=subprocess.Popen(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf8',errors='replace',env=dict(os.environ,PYTHONIOENCODING='utf-8'))
  for line in p.stdout:report.write(line);report.flush();log(line.rstrip())
  if p.wait():raise ValueError('Setup failed. Log: '+str(dest.with_suffix('.log')))
 return 'Setup complete: '+str(dest)
