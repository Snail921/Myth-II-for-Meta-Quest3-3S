import unittest,tempfile,json,shlex,hashlib
from pathlib import Path
from unittest.mock import patch
from setup_deploy import deploy,BASE,ROOT
H=lambda b:hashlib.sha256(b).hexdigest()
class DeviceFake:
 def __init__(self,fail=False):self.serial='fresh-quest';self.files={BASE+'/SharedAssets/old':b'old',BASE+'/external-layout.version':b'1'};self.fail=fail;self.log=lambda s:None;self.seed_hash=(ROOT/'tools/runtime-seed.sha256').read_text().strip()
 def ready(self):pass
 def exists(self,p):return p in self.files or any(n.startswith(p+'/') for n in self.files)
 def sha(self,p):return self.seed_hash if p.endswith('/runtime-seed.tar') else H(self.files[p])
 def call(self,*a):
  if a[0]=='push':self.files[a[2]]=Path(a[1]).read_bytes()
  return ''
 def shell(self,c):
  if c.startswith('cat '):return self.files[shlex.split(c)[1]].decode()
  return 'Filesystem Size Used Available Use Mounted\n/dev/fuse 999999999 0 999999999 0 /sdcard' if c.startswith('df ') else ''
 def move(self,a,b):
  if self.fail and a.startswith(BASE+'/.setup-') and '/SharedAssets' in a and b==BASE+'/SharedAssets':self.fail=False;raise RuntimeError('publish failure')
  if self.exists(b):raise RuntimeError('Destination exists')
  names=[n for n in self.files if n==a or n.startswith(a+'/')]
  if not names:raise RuntimeError('Missing move source '+a)
  for n in names:self.files[b+n[len(a):]]=self.files.pop(n)
class Tests(unittest.TestCase):
 def scenario(self,fail):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp);seed=root/'seed.tar';seed.write_bytes(b'seed');files={}
   for n in ('Game/Myth II.exe','SharedAssets/font','LevelPacks/soulblighter/catalog.tsv'):
    p=root/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(n.encode());files[n]={'size':p.stat().st_size,'sha256':H(n.encode())}
   device=DeviceFake(fail)
   with patch('setup_deploy.validated',return_value={'files':files}),patch('setup_deploy.digest',return_value=device.seed_hash):
    if fail:
     with self.assertRaisesRegex(RuntimeError,'publish failure'):deploy(root,seed,device)
     self.assertFalse(device.exists(BASE+'/Game'));self.assertEqual(device.files[BASE+'/SharedAssets/old'],b'old')
    else:
     deploy(root,seed,device);self.assertTrue(device.exists(BASE+'/Game/Myth II.exe'));self.assertTrue(device.exists(BASE+'/runtime-seed.tar'));self.assertFalse(device.exists(BASE+'/SharedAssets/old'))
   state=json.loads(next(root.glob('deployment-*.json')).read_text());self.assertEqual(state['status'],'failed_rolled_back' if fail else 'complete')
 def test_publish(self):self.scenario(False)
 def test_rollback(self):self.scenario(True)
if __name__=='__main__':unittest.main()
