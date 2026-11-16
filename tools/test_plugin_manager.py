import hashlib,json,shlex,tempfile,tarfile,io,unittest
from pathlib import Path
from unittest.mock import patch
from plugin_manager_backend import Device,REMOTE,GAME
H=lambda b:hashlib.sha256(b).hexdigest()
class Fake(Device):
 def __init__(self,req,fail=False):
  super().__init__(log=lambda s:None);self.fail=fail;self.old=REMOTE+'/previous-pack';self.files={GAME+'/Myth II.exe':b'exe',GAME+'/plugins/example':b'old-plugin',self.old+'/source-files.json':json.dumps(req).encode(),self.old+'/old':b'old'};self.dirs={REMOTE,self.old};self.published=False
 def ready(self):pass
 def exists(self,p):return p in self.files or p in self.dirs
 def sha(self,p):return H(self.files[p])
 def call(self,*args):
  if args[0]=='push':self.files[args[2]]=Path(args[1]).read_bytes()
  return ''
 def move(self,a,b):
  if self.fail and '/.install-' in a and b.startswith(REMOTE+'/plugin-'):self.fail=False;raise RuntimeError('simulated publish failure')
  if self.exists(b):raise ValueError('existing destination')
  if a in self.files:self.files[b]=self.files.pop(a)
  else:
   self.dirs.discard(a);self.dirs.add(b)
   for p in list(self.files):
    if p.startswith(a+'/'):self.files[b+p[len(a):]]=self.files.pop(p)
 def shell(self,c):
  t=shlex.split(c)
  if t[0]=='df':return 'Filesystem Size Used Available Use Mounted\n/dev/fuse 999999999 0 999999999 0 /sdcard'
  if t[0]=='find':return self.old
  if t[0]=='cat':return self.files[t[1]].decode()
  if t[0]=='mkdir':self.dirs.add(t[-1])
  if t[0]=='tar':
   with tarfile.open(fileobj=io.BytesIO(self.files[t[2]])) as f:
    for m in f.getmembers():self.files[t[4]+'/'+m.name]=f.extractfile(m).read()
  if t[0]=='rm':self.files.pop(t[-1],None)
  return ''
class Tests(unittest.TestCase):
 def scenario(self,fail):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);bundle=root/'bundle';bundle.mkdir();(bundle/'catalog.tsv').write_text('test');plugin=root/'example';plugin.write_bytes(b'new-plugin')
   item={'path':'plugins/example','sha256':H(b'new-plugin'),'runtime_identity':'plugin'};req={'engine_sha256':H(b'exe'),'files':[item],'active_plugins':['plugin']}
   dev=Fake(req,fail)
   with patch('plugin_manager_backend.bundle_inputs',return_value=({'levels':[{}]},req,item)):
    if fail:
     with self.assertRaisesRegex(RuntimeError,'publish failure'):dev.deploy(bundle,plugin,root/'logs')
     self.assertEqual(dev.files[GAME+'/plugins/example'],b'old-plugin');self.assertEqual(dev.files[dev.old+'/old'],b'old')
    else:
     journal=dev.deploy(bundle,plugin,root/'logs');state=json.loads(journal.read_text(encoding='utf8'));self.assertEqual(state['status'],'complete');self.assertEqual(len(state['backups']),2);self.assertEqual(dev.files[GAME+'/plugins/example'],b'new-plugin');self.assertFalse(dev.exists(dev.old))
   self.assertFalse(list((root/'logs').glob('*.tar')))
 def test_publish_and_backups(self):self.scenario(False)
 def test_publish_failure_restores_both(self):self.scenario(True)
 def test_running_game_rejected(self):
  dev=Device(serial='quest')
  with patch.object(dev,'devices',return_value=['quest']),patch.object(dev,'shell',side_effect=['package:/app/base.apk','Myth II.exe']):
   with self.assertRaisesRegex(ValueError,'原作ゲーム'):dev.ready()
if __name__=='__main__':unittest.main()
