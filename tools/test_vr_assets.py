"""Validate source animation metadata and decoded pixels against captured runtime references."""
import json,unittest,struct
from pathlib import Path
from prepare_vr_assets import decode,resolved_instance
ROOT=Path(__file__).resolve().parents[1]
class AssetTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.scene=json.loads((ROOT/'build/scene-bridge/scene.json').read_text())
  cls.collections={a['runtime_index']:decode((ROOT/'build/scene-bridge'/a['file']).read_bytes()) for a in cls.scene['assets']}
 def test_runtime_animation_references(self):
  for key,v in self.scene['animations'].items():
   c,n=map(int,key.split(':'));flags,scale,refs=self.collections[c][2][n]
   with self.subTest(state=key):
    self.assertEqual(flags,v['flags'])
    self.assertEqual(refs,v['view_instance_indices'])
    self.assertEqual(scale,v['pixels_to_world_raw']/65536)
 def test_all_bitmap_lengths_and_transparency(self):
  transparent=0
  for images,_,_ in self.collections.values():
   for w,h,p in images:
    self.assertEqual(len(p),w*h*4)
    transparent+=p[3::4].count(0)
  self.assertGreater(transparent,10000)
 def test_truncated_assets_rejected(self):
  asset=ROOT/'build/scene-bridge'/self.scene['assets'][0]['file']
  for n in [4,319,1024]:
   with self.subTest(length=n),self.assertRaises((ValueError,IndexError,struct.error)):
    decode(asset.read_bytes()[:n])
 def test_mirrored_missing_views(self):
  count=0
  for col,(images,instances,sequences) in self.collections.items():
   for flags,scale,frames in sequences:
    for row in frames:
     for view,index in enumerate(row):
      before=instances[index];after=resolved_instance(images,instances,flags,row,view)
      if before[3]>=0:self.assertEqual(after,before)
      elif after[3]>=0:
       source=instances[row[len(row)-view]];w=images[source[3]][0]
       self.assertEqual(after,(source[0]^1,w-source[1],source[2],source[3]));count+=1
  self.assertGreater(count,0)
  images,instances,sequences=self.collections[25]
  flags,_,frames=sequences[5]
  for row in frames:
   for view in range(8):self.assertGreaterEqual(resolved_instance(images,instances,flags,row,view)[3],0)
if __name__=='__main__':unittest.main()
