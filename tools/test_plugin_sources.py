"""Archive resolver regression tests using small synthetic archives, no original data."""
import unittest,tempfile,struct
from pathlib import Path
from plugin_sources import archive,resolve

def make(path,value=b'abcd',duplicate=False,bad_offset=False):
 h=bytearray(128);struct.pack_into('>HH',h,0,1,65535);struct.pack_into('>HH',h,100,1,2 if duplicate else 1);h[124:]=b'dng2'
 entry=bytearray(112);entry[:4]=b'test';entry[16:18]=b'01';entry[48:52]=b'Test'
 r=bytearray(64);r[4:8]=b'Test';r[36:44]=b'meshtest';struct.pack_into('>II',r,44,0 if bad_offset else 128+112+64*(2 if duplicate else 1),len(value));r[60:64]=b'mth2'
 path.write_bytes(h+entry+r+(r if duplicate else b'')+value)

class Tests(unittest.TestCase):
 def test_bounds_duplicates_and_signature(self):
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp)/'plugin';make(p);a=archive(p);self.assertEqual(a['entries'][0]['name'],'Test');self.assertEqual(a['tags'][0]['size'],4)
   make(p,bad_offset=True)
   with self.assertRaises(ValueError):archive(p)
   make(p,duplicate=True)
   with self.assertRaises(ValueError):archive(p)
   make(p);p.write_bytes(p.read_bytes()[:-1])
   with self.assertRaises(ValueError):archive(p)
   p.write_bytes(bytes(128))
   with self.assertRaises(ValueError):archive(p)
 def test_explicit_order_and_provenance(self):
  with tempfile.TemporaryDirectory() as tmp:
   a,b=Path(tmp)/'base',Path(tmp)/'override';make(a,b'base');make(b,b'patch');aa,bb=archive(a),archive(b)
   forward=resolve([aa,bb],[bb['path']]);backward=resolve([bb,aa],[aa['path']])
   self.assertEqual(forward['tags'][0][2],bb['path']);self.assertEqual(backward['tags'][0][2],aa['path']);self.assertNotEqual(aa['sha256'],bb['sha256']);self.assertEqual(forward['overrides'][0]['from'],aa['path'])
if __name__=='__main__':unittest.main()
