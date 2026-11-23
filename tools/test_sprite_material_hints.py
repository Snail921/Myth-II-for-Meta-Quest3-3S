import unittest,tempfile
from pathlib import Path
from PIL import Image
from sprite_material import Project,Frame
from sprite_material_hints import validate_hints
class MaterialHintsTests(unittest.TestCase):
 def test_roundtrip_and_runtime_unchanged(self):
  p=Project([Frame('sample',Image.new('RGBA',(3,3),(100,80,60,255)))])
  with tempfile.TemporaryDirectory() as d:
   d=Path(d);p.export_runtime(d/'before.mvrnm')
   p.material_hints=[{'name':'','colors':[None]*3} for _ in range(5)]
   p.material_hints[0]={'name':'鎧','colors':[[255,128,0],None,[1,2,3]]}
   p.save(d/'test.smproj');q=Project.load(d/'test.smproj')
   self.assertEqual(q.material_hints,validate_hints(p.material_hints))
   self.assertEqual(q.material_hints[0]['samples'][:3],[[[255,128,0]],[],[[1,2,3]]])
   self.assertEqual(len(q.material_hints),10)
   q.export_runtime(d/'after.mvrnm');self.assertEqual((d/'before.mvrnm').read_bytes(),(d/'after.mvrnm').read_bytes())
 def test_validation(self):
  p=Project();p.material_hints[0]['colors'][0]=[256,0,0]
  with self.assertRaises(ValueError):validate_hints(p.material_hints)
 def test_independent_defaults(self):
  p,q=Project(),Project();p.material_hints[0]['name']='a'
  self.assertEqual(q.material_hints[0]['name'],'')
if __name__=='__main__':unittest.main()
