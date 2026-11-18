import contextlib,io,json,struct,tempfile,unittest,zipfile
from pathlib import Path
from PIL import Image
from normal_library import export_library,restore_library
class NormalLibraryTest(unittest.TestCase):
    def test_roundtrip_and_no_source(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);src=r/'packs';src.mkdir();owned=r/'owned';owned.mkdir()
            source=bytes([77,123,231,255])*4;normal=bytes([128,128,255,255])*4
            original=b'MVRNORM1'+struct.pack('<III',1,2,2)+source+normal
            (src/'unit.mvrnm').write_bytes(original);Image.frombytes('RGBA',(2,2),source).save(owned/'unit.png')
            with contextlib.redirect_stdout(io.StringIO()):
                export_library(src,r/'library.zip');restore_library(r/'library.zip',owned,r/'out')
            self.assertEqual((r/'out/unit.mvrnm').read_bytes(),original)
            with zipfile.ZipFile(r/'library.zip') as z:
                for name in z.namelist():self.assertNotIn(source,z.read(name))
                z.extractall(r/'extracted')
            with contextlib.redirect_stdout(io.StringIO()):restore_library(r/'extracted',owned,r/'out2')
            self.assertEqual((r/'out2/unit.mvrnm').read_bytes(),original)
    def test_reject_path(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d)
            with zipfile.ZipFile(r/'bad.zip','w') as z:z.writestr('library.json',json.dumps({'version':1,'packs':[{'name':'../escape.mvrnm','frames':[]}]}))
            with self.assertRaises(ValueError):restore_library(r/'bad.zip',r,r/'out')
    def test_reject_corrupt_normal(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);src=r/'packs';src.mkdir();owned=r/'owned';owned.mkdir()
            source=bytes([77,123,231,255]);(src/'unit.mvrnm').write_bytes(b'MVRNORM1'+struct.pack('<III',1,1,1)+source+bytes([128,128,255,255]));Image.frombytes('RGBA',(1,1),source).save(owned/'unit.png')
            with contextlib.redirect_stdout(io.StringIO()):export_library(src,r/'library.zip')
            with zipfile.ZipFile(r/'library.zip') as z,zipfile.ZipFile(r/'bad.zip','w') as out:
                for name in z.namelist():out.writestr(name,b'bad!' if name.startswith('normals/') else z.read(name))
            with self.assertRaises(ValueError),contextlib.redirect_stdout(io.StringIO()):restore_library(r/'bad.zip',owned,r/'out')
            self.assertFalse((r/'out/unit.mvrnm').exists())
if __name__=='__main__':unittest.main()
