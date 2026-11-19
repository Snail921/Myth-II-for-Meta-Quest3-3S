import json
import tempfile
import unittest
import zipfile
from pathlib import Path
import numpy as np
from PIL import Image
from sprite_material import Frame, Project


class MaterialTests(unittest.TestCase):
    def test_shape_layer_roundtrip_and_mask_priority(self):
        f=Frame('shape',Image.new('RGBA',(4,4),(100,100,100,255)))
        f.correction=Image.new('L',(4,4),210)
        f.correction_mask=Image.new('L',(4,4),0)
        f.correction_mask.putpixel((1,1),255)
        p=Project([f])
        self.assertEqual(p.height(f).getpixel((1,1)),210)
        self.assertEqual(p.height(f).getpixel((0,0)),100)
        p.correction_enabled=False
        self.assertEqual(p.height(f).getpixel((1,1)),100)
        p.correction_enabled=True
        f.mask.putpixel((1,1),255)
        self.assertEqual(p.height(f).getpixel((1,1)),128)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'shape.smproj';p.save(path);q=Project.load(path)
            self.assertEqual(q.frames[0].correction.tobytes(),f.correction.tobytes())
            self.assertEqual(q.frames[0].correction_mask.tobytes(),f.correction_mask.tobytes())
            self.assertEqual(q.height(q.frames[0]).tobytes(),p.height(f).tobytes())

    def test_runtime_pack_preserves_matching_source(self):
        import struct
        f=Frame('unit',Image.new('RGBA',(4,3),(80,120,160,255)))
        p=Project([f])
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'unit.mvrnm';p.export_runtime(path);data=path.read_bytes()
            self.assertEqual(data[:8],b'MVRNORM2')
            self.assertEqual(struct.unpack_from('<III',data,8),(1,4,3))
            import hashlib
            self.assertEqual(data[20:52],hashlib.sha256(struct.pack('<II',4,3)+f.image.tobytes()).digest())
            self.assertEqual(data[52:],p.normal(f).tobytes())

    def test_flat_and_transparent_boundary(self):
        a=np.zeros((7,7,4),dtype=np.uint8);a[1:6,1:6]=(100,100,100,255)
        f=Frame('flat',Image.fromarray(a));p=Project([f]);n=np.asarray(p.normal(f))
        self.assertTrue(np.all(n[:,:,:3]==(128,128,255)))
        self.assertTrue(np.array_equal(n[:,:,3],a[:,:,3]))

    def test_slopes_and_mask_fill(self):
        a=np.zeros((5,5,4),dtype=np.uint8);a[:,:,3]=255
        for x in range(5):a[:,x,:3]=x*50
        f=Frame('ramp',Image.fromarray(a));p=Project([f]);n=np.asarray(p.normal(f))
        self.assertLess(n[2,2,0],128);self.assertEqual(n[2,2,1],128)
        f.mask=Image.new('L',(5,5),255);p.height_value=80
        self.assertTrue(np.all(np.asarray(p.height(f))==80))
        self.assertTrue(np.all(np.asarray(p.normal(f))[:,:,:3]==(128,128,255)))

    def test_mask_roundtrip_and_export(self):
        f=Frame('unit',Image.new('RGBA',(4,4),(250,240,230,255)),{'collection_tag':'77617272','uses':[{'direction':1}]})
        p=Project([f]);p.colors=[[250,240,230]];p.tolerance=0;p.apply_masks()
        self.assertEqual(f.mask.getpixel((0,0)),255)
        f.mask.putpixel((0,0),0)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'unit.smproj';p.save(path);loaded=Project.load(path)
            self.assertEqual(loaded.frames[0].mask.tobytes(),f.mask.tobytes())
            self.assertEqual(loaded.frames[0].identity,f.identity)
            self.assertEqual(loaded.settings(),p.settings())
            export=Path(d)/'output.zip';loaded.export(export)
            with zipfile.ZipFile(export) as z:
                m=json.loads(z.read('manifest.json'));self.assertFalse(m['runtime_ready'])
                self.assertEqual(m['frames'][0]['identity'],f.identity)
                self.assertEqual(len(z.namelist()),4)

    def test_failed_save_preserves_previous(self):
        p=Project([Frame('test',Image.new('RGBA',(2,2)))])
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'test.smproj';p.save(path);before=path.read_bytes()
            def fail(*args):raise RuntimeError('simulated disk failure')
            with self.assertRaises(RuntimeError):p.save(path,fail)
            self.assertEqual(path.read_bytes(),before)
            self.assertEqual(list(Path(d).iterdir()),[path])


if __name__=='__main__':unittest.main()
