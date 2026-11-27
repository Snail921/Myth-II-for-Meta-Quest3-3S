import unittest,tempfile,struct
from pathlib import Path
from terrain_base import export_terrain_base
class TerrainBaseTest(unittest.TestCase):
 def test_palette_and_shade_independence(self):
  palette=[(i*257,i*257,i*257) for i in range(256)]
  pixels=bytes([64,64,64,255])*65536
  settings=dict(global_tint_fraction=0,global_tint_color="\0"*6)
  with tempfile.TemporaryDirectory() as temp:
   export_terrain_base(b"",[(256,256,pixels),(256,256,bytes(262144))],palette,settings,256,256,temp)
   first=(Path(temp)/'terrain-base.pack').read_bytes()
   export_terrain_base(b"",[(256,256,pixels),(256,256,bytes([255])*262144)],palette,settings,256,256,temp)
   self.assertEqual(first,(Path(temp)/'terrain-base.pack').read_bytes())
   self.assertEqual(first[:16],b'MYTHBC01'+struct.pack('<II',256,256))
   self.assertEqual(first[1040:],bytes([64,64,64,255])*65536)
   self.assertTrue((Path(temp)/'terrain-base.png').is_file())
if __name__=='__main__':unittest.main()
