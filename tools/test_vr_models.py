import json,struct,unittest,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ModelTests(unittest.TestCase):
 def test_geometry_and_windmill_assembly(self):
  b=(ROOT/'build/vr/assets/models.pack').read_bytes();self.assertIn(b[:8],(b'MYTHMD01',b'MYTHMD02',b'MYTHMD03'));nt,nc=struct.unpack_from('<II',b,8);p=16
  for _ in range(nt):
   w,h=struct.unpack_from('<II',b,p);p+=8+w*h*4+(32 if b[:8]==b'MYTHMD03' else 0)
  self.assertGreaterEqual(len(b),p+nc*64)
  records=json.loads((ROOT/'research/reports/vr-models.json').read_text());centers={};offset=0
  for record in records:
   pts=[]
   for i in range(record['triangles']):
    vals=struct.unpack_from('<I15f',b,p+(offset+i)*64);self.assertLess(vals[0],nt)
    for j in range(3):
     point=vals[1+j*5:4+j*5];self.assertTrue(all(math.isfinite(v) for v in point));pts.append(point)
   centers[record['tag']]=[(min(v[k] for v in pts)+max(v[k] for v in pts))/2 for k in range(3)];offset+=record['triangles']
  self.assertEqual(offset,nc)
  # Blade marker 10002 must be adjacent to the original tower, not detached by its local origin.
  tower,blade=centers['wind'],centers['01wb']
  self.assertLess(math.hypot(tower[0]-blade[0],tower[1]-blade[1]),3)
if __name__=='__main__':unittest.main()
