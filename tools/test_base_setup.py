import unittest,tempfile,json,sys,zipfile
from pathlib import Path
from unittest.mock import patch
from setup_deploy import validated
from base_setup import prepare_game,ENGINE
from plugin_sources import profile
ROOT=Path(__file__).resolve().parents[1]
class SetupTests(unittest.TestCase):
 def test_setup_path_traversal_rejected(self):
  for bad in ('../outside','/Game/x','Game/../../outside','Game\\x','Game/C:x'):
   with self.subTest(path=bad),tempfile.TemporaryDirectory() as temp:
    p=Path(temp);(p/'setup-manifest.json').write_text(json.dumps({'version':1,'files':{bad:{'size':1,'sha256':'0'*64}}}))
    with self.assertRaisesRegex(ValueError,'path'):validated(p)
 def test_empty_manifest_rejected(self):
  with tempfile.TemporaryDirectory() as temp:
   p=Path(temp);(p/'setup-manifest.json').write_text('{"version":1,"files":{}}')
   with self.assertRaises(ValueError):validated(p)
 def test_source_profile_independent_of_research_inventory(self):
  game=ROOT/'build/base-setup-verification/Game'
  old=Path.read_text
  def read(p,*a,**kw):
   if 'tag-inventory.json' in str(p):raise AssertionError('Legacy research dependency')
   return old(p,*a,**kw)
  with patch.object(Path,'read_text',read):result=profile([],game)
  self.assertEqual(len(result['archives']),10);self.assertEqual(result['selected'],[])
 def test_wrong_executable_rejected(self):
  with tempfile.TemporaryDirectory() as temp:
   p=Path(temp);(p/'Myth II.exe').write_bytes(b'wrong')
   with self.assertRaises(ValueError):profile([],p)
 def test_existing_output_preserved(self):
  with tempfile.TemporaryDirectory() as temp:
   p=Path(temp);(p/'save').write_bytes(b'keep')
   with self.assertRaises(ValueError):prepare_game('absent','absent',p,'absent')
   self.assertEqual((p/'save').read_bytes(),b'keep')
 def test_external_apk(self):
  with zipfile.ZipFile(ROOT/'build/myth-standalone-185-external.apk') as z:
   self.assertFalse(any(n.startswith('assets/engine185/') for n in z.namelist()))
   self.assertEqual(z.read('assets/levels/catalog.tsv'),b'')
   for n in ('info-font.rgba','info-decoration.pack','scene.pack','models.pack','sprite-indices.pack','dialog-templates.pack'):
    self.assertNotIn('assets/'+n,z.namelist())
if __name__=='__main__':unittest.main()
