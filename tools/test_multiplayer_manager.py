import json,tempfile,unittest,shlex
from pathlib import Path
from unittest.mock import patch
from level_modes import level_mode
import plugin_manager_backend as backend

class ManagerTests(unittest.TestCase):
 def test_modes(self):
  self.assertEqual(level_mode(0x110000,'l1  '),'single')
  self.assertEqual(level_mode(0x100001,'85gy'),'multiplayer')
  self.assertEqual(level_mode(0x110001,'dual'),'both')
  self.assertEqual(level_mode(0,'00tm'),'single')
  self.assertEqual(level_mode(0x100000,'none'),'unknown')
 def test_base_deploy_and_rollback(self):
  for fail in (False,True):
   with self.subTest(fail=fail),tempfile.TemporaryDirectory() as temp:
    bundle=Path(temp)/'bundle';bundle.mkdir();req={'active_plugins':[],'engine_sha256':'e','files':[{'path':'tags/test','sha256':'a'*64}]}
    (bundle/'source-files.json').write_text(json.dumps(req));(bundle/'catalog.tsv').write_text('test')
    base=backend.REMOTE+'/soulblighter';plugin=backend.REMOTE+'/plugin-other'
    class Fake(backend.Device):
     def __init__(self):self.log=lambda s:None;self.moves=[];self.paths={backend.GAME+'/Myth II.exe',base,base+'/source-files.json',plugin,plugin+'/source-files.json'};self.failed=False
     def ready(self):pass
     def exists(self,p):return p in self.paths
     def sha(self,p):
      if p.endswith('Myth II.exe'):return 'e'
      if p.endswith('tags/test'):return 'a'*64
      return backend.digest(bundle/p.rsplit('/',1)[1])
     def shell(self,c):
      if c.startswith('df '):return 'fs total used 999999999 available'
      if c.startswith('find '):return base+'\n'+plugin
      if c.startswith('cat '):return json.dumps(req if shlex.split(c)[1].startswith(base+'/') else {'active_plugins':['other'],'files':[]})
      return ''
     def call(self,*a,**kw):return ''
     def move(self,a,b):
      if fail and a.startswith(backend.REMOTE+'/.install-') and b==base and not self.failed:self.failed=True;raise RuntimeError('publish failed')
      self.moves.append((a,b));self.paths.discard(a);self.paths.add(b)
    d=Fake()
    with patch.object(backend,'verify',return_value={'levels':['test']}):
     if fail:
      with self.assertRaises(RuntimeError):d.deploy(bundle,'',Path(temp)/'logs')
     else:d.deploy(bundle,'',Path(temp)/'logs')
     with self.assertRaises(ValueError):backend.bundle_inputs(bundle,Path(temp)/'wrong-plugin')
    self.assertIn(base,d.paths);self.assertIn(plugin,d.paths)
    self.assertTrue(all(not a.startswith(backend.GAME+'/') and not b.startswith(backend.GAME+'/') for a,b in d.moves))
    if fail:self.assertTrue(any('.backup-' in a and b==base for a,b in d.moves))

if __name__=='__main__':unittest.main()
