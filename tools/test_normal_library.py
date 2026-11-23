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
    def test_hash_pack_conversion_and_direct_unpack(self):
        from normal_library import convert_packs,hashed_frames
        from sprite_material_deploy import validate
        import hashlib
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);src=r/'packs';src.mkdir()
            source=bytes(range(80));normal=bytes([128,128,255,255])*20
            (src/'unit.mvrnm').write_bytes(b'MVRNORM1'+struct.pack('<III',1,4,5)+source+normal)
            convert_packs(src,r/'new');pack=(r/'new/unit.mvrnm').read_bytes()
            self.assertEqual(pack,b'MVRNORM2'+struct.pack('<III',1,4,5)+hashlib.sha256(struct.pack('<II',4,5)+source).digest()+normal)
            validate(r/'new/unit.mvrnm')
            with contextlib.redirect_stdout(io.StringIO()):
                export_library(r/'new',r/'library.zip');restore_library(r/'library.zip',None,r/'direct')
            self.assertEqual((r/'direct/unit.mvrnm').read_bytes(),pack)
            for data in (pack[:-1],pack+b'x'):
                (r/'bad.mvrnm').write_bytes(data)
                with self.assertRaises(ValueError):validate(r/'bad.mvrnm')

    def test_packed_material_library_preserves_channels_and_format(self):
        from sprite_material import Frame,Project
        from sprite_material_deploy import validate
        from normal_library import convert_packs
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);src=r/'packs';src.mkdir();owned=r/'owned';owned.mkdir()
            frame=Frame('unit',Image.new('RGBA',(3,2),(90,30,60,255)))
            frame.pbr_enabled=True
            frame.metallic_smoothness=Image.new('L',(3,2),153)
            frame.thickness=Image.new('L',(3,2),77)
            frame.image.save(owned/'unit.png')
            Project([frame]).export_runtime(src/'unit.mvrnm')
            data=(src/'unit.mvrnm').read_bytes();validate(src/'unit.mvrnm')
            self.assertEqual(data[56:],bytes([128,128,77,76])*6)
            with contextlib.redirect_stdout(io.StringIO()):
                convert_packs(src,r/'converted')
                export_library(src,r/'library.zip')
                restore_library(r/'library.zip',None,r/'out')
                restore_library(r/'library.zip',owned,r/'verified')
            for folder in ('converted','out','verified'):
                self.assertEqual((r/folder/'unit.mvrnm').read_bytes(),data)
            for corrupt in (data[:-1],data+b'x'):
                (r/'bad.mvrnm').write_bytes(corrupt)
                with self.assertRaises(ValueError):validate(r/'bad.mvrnm')

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
