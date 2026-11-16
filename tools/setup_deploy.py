"""Verified, rollback-capable installation of a PC-generated base setup."""
import json,re,shlex,uuid
from pathlib import Path,PurePosixPath
from plugin_manager_backend import digest,PACKAGE,ROOT
from verify_level_bundle import verify
BASE='/sdcard/Android/data/'+PACKAGE+'/files'
def validated(folder):
 folder=Path(folder).resolve()
 if any(p.is_symlink() for top in ('Game','SharedAssets','LevelPacks','Skyboxes') for p in (folder/top).rglob('*')):raise ValueError('Setup symlink')
 m=json.loads((folder/'setup-manifest.json').read_text(encoding='utf8'))
 if m.get('version')!=1 or not m.get('files'):raise ValueError('Invalid setup manifest')
 for name,spec in m['files'].items():
  parts=PurePosixPath(name).parts
  if not parts or parts[0] not in ('Game','SharedAssets','LevelPacks','Skyboxes') or '\\' in name or ':' in name or '\0' in name or '..' in parts or name.startswith('/'):raise ValueError('Invalid setup path')
  unresolved=folder/name
  if unresolved.is_symlink():raise ValueError('Setup symlink')
  p=unresolved.resolve()
  if not p.is_relative_to(folder) or p.is_symlink() or p.stat().st_size!=spec['size'] or digest(p)!=spec['sha256']:raise ValueError('Setup checksum: '+name)
 actual={p.relative_to(folder).as_posix() for top in ('Game','SharedAssets','LevelPacks','Skyboxes') for p in (folder/top).rglob('*') if p.is_file()}
 if actual!=set(m['files']):raise ValueError('Setup has unlisted or missing files')
 verify(folder/'LevelPacks/soulblighter')
 return m

def deploy(folder,seed,device):
 folder=Path(folder).resolve()
 if any(p.is_symlink() for top in ('Game','SharedAssets','LevelPacks','Skyboxes') for p in (folder/top).rglob('*')):raise ValueError('Setup symlink')
 m=validated(folder);seed=Path(seed)
 expected=(ROOT/'tools/runtime-seed.sha256').read_text().strip()
 if not seed.is_file() or digest(seed)!=expected:raise ValueError('Select the supported MythVR-runtime-seed.tar')
 device.ready()
 if not device.exists(BASE+'/external-layout.version') or device.shell('cat '+shlex.quote(BASE+'/external-layout.version'))!='1':raise ValueError('Open the external-data APK once on the selected Quest before deployment.')
 if device.exists(BASE+'/Game'):raise ValueError('Game already exists. Existing game data is preserved; use a fresh device for initial setup.')
 token=uuid.uuid4().hex;stage=BASE+'/.setup-'+token;backup=BASE+'/.setup-backup-'+token
 free=int(device.shell('df -k /sdcard').splitlines()[-1].split()[3])*1024
 if free<sum(s['size'] for s in m['files'].values())+seed.stat().st_size+3*1024**3:raise ValueError('Insufficient Quest storage')
 changes=[];published=[];journal=folder/('deployment-'+token+'.json')
 def record(status):journal.write_text(json.dumps(dict(status=status,serial=device.serial,backups=changes,published=published),indent=2),encoding='utf8')
 record('staging');device.shell('mkdir -p '+shlex.quote(stage))
 try:
  for i,(name,spec) in enumerate(m['files'].items(),1):
   remote=stage+'/'+name;device.shell('mkdir -p '+shlex.quote(remote.rsplit('/',1)[0]));device.call('push',str(folder/name),remote)
   if device.sha(remote)!=spec['sha256']:raise ValueError('Transfer checksum: '+name)
   device.log('Setup transfer (%03d/%03d): %s'%(i,len(m['files']),name))
  device.call('push',str(seed),stage+'/runtime-seed.tar')
  if device.sha(stage+'/runtime-seed.tar')!=expected:raise ValueError('Runtime transfer checksum')
  device.ready();device.call('shell','am','force-stop',PACKAGE)
  # Only the base pack is replaced: other gameplay packs and user saves survive.
  targets=[('Game',BASE+'/Game'),('SharedAssets',BASE+'/SharedAssets'),('LevelPacks/soulblighter',BASE+'/LevelPacks/soulblighter'),('runtime-seed.tar',BASE+'/runtime-seed.tar')]
  if (folder/'Skyboxes').is_dir():targets.append(('Skyboxes',BASE+'/Skyboxes'))
  for relative,target in targets:
   # Existing Game includes saves/preferences: never replace it during base setup.
   if relative=='Game' and device.exists(target):raise ValueError('Game already exists. Base setup is for a fresh device; existing game data has been preserved.')
   device.shell('mkdir -p '+shlex.quote(target.rsplit('/',1)[0]))
   if device.exists(target):
    saved=backup+'/'+relative;device.shell('mkdir -p '+shlex.quote(saved.rsplit('/',1)[0]));device.move(target,saved);changes.append([target,saved]);record('publishing')
   device.move(stage+'/'+relative,target);published.append([target,stage+'/'+relative]);record('publishing')
  record('complete')
 except Exception:
  for target,old in reversed(published):
   if device.exists(target):device.shell('mkdir -p '+shlex.quote(old.rsplit('/',1)[0]));device.move(target,old)
  for target,saved in reversed(changes):
   if not device.exists(target):device.move(saved,target)
  record('failed_rolled_back');raise
 device.call('shell','am','start','-n',PACKAGE+'/.StandaloneActivity')
 return 'Setup deployed. Open Initial setup on Quest. Log: '+str(journal)
