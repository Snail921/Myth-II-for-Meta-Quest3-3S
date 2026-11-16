import unittest,sys,json,tempfile,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from engine_profile import load_profile,parse_pool,ROOT
class ProfileTest(unittest.TestCase):
 def test_versions(self):
  old=load_profile(ROOT/'research/extracted/update-1.8.1/Myth II.exe');new=load_profile(ROOT/'research/extracted/update-1.8.5/Myth II.exe')
  self.assertEqual(old['record_sizes']['actor'],216);self.assertEqual(new['record_sizes']['actor'],220);self.assertFalse(new['capabilities']['commands']);self.assertFalse(new['capabilities']['production_bridge'])
 def test_unknown_rejected(self):
  with tempfile.TemporaryDirectory(dir=ROOT/'build') as d:
   p=Path(d)/'unknown.exe';p.write_bytes(b'MZ unknown');self.assertRaises(ValueError,load_profile,p)
 def test_pool_layouts(self):
  for p in json.loads((ROOT/'src/engine/profiles.json').read_text())['profiles']:
   raw=bytearray(64);raw[:7]=b'monster';layout=p['pool_layout']
   struct.pack_into('<H',raw,layout['stride'],p['record_sizes']['actor']);struct.pack_into('<H',raw,layout['count'],17);struct.pack_into('<I',raw,layout['data'],0x12340000)
   pool=parse_pool(raw,layout);self.assertEqual(pool['count'],17);self.assertEqual(pool['stride'],p['record_sizes']['actor']);self.assertEqual(pool['data'],0x12340000)
if __name__=='__main__':unittest.main()
