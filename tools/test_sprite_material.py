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
        f.pbr_enabled=True
        p=Project([f])
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'unit.mvrnm';p.export_runtime(path);data=path.read_bytes()
            self.assertEqual(data[:8],b'MVRMAT05')
            self.assertEqual(struct.unpack_from('<III',data,8),(1,4,3))
            import hashlib
            self.assertEqual(data[24:56],hashlib.sha256(struct.pack('<II',4,3)+f.image.tobytes()).digest())
            packed=np.frombuffer(data[56:],dtype=np.uint8).reshape(3,4,4)
            self.assertTrue(np.array_equal(packed[:,:,:2],np.asarray(p.normal(f))[:,:,:2]))
            self.assertTrue(np.all(packed[:,:,2]==255))
            self.assertTrue(np.all(packed[:,:,3]==0))

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
                self.assertEqual(set(z.namelist()), {'manifest.json', 'height/00000.png', 'normal/00000.png', 'mask/00000.png', 'metallic_smoothness/00000.png', 'metallic/00000.png', 'thickness/00000.png'})

    def test_material_layers_and_recovery(self):
        f=Frame('unit',Image.new('RGBA',(4,4),(30,40,50,255)))
        p=Project([f]);normal=p.normal(f).tobytes()
        self.assertEqual(f.metallic_smoothness.getextrema(),(0,0))
        self.assertEqual(f.thickness.getextrema(),(255,255))
        f.metallic_smoothness.putpixel((1,1),191);f.thickness.putpixel((2,2),77)
        f.thickness.putpixel((0,0),0)
        p.material_hints[0]['name']='armor'
        state=dict(index=0,view='thickness',dirty=True)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'session.smproj';p.save(path,recovery=state);q=Project.load(path)
            self.assertEqual(q.frames[0].metallic_smoothness.getpixel((1,1)),191)
            self.assertEqual(q.frames[0].thickness.getpixel((2,2)),77)
            self.assertEqual(q.frames[0].thickness.getpixel((0,0)),0)
            self.assertEqual(q.normal(q.frames[0]).tobytes(),normal)
            self.assertEqual(q.material_hints[0]['name'],'armor')
            with zipfile.ZipFile(path) as z:self.assertEqual(json.loads(z.read('editor-session.json')),state)
            # Simulate an old project without the new layers.
            old=Path(d)/'old.smproj'
            with zipfile.ZipFile(path) as src,zipfile.ZipFile(old,'w') as dst:
                for name in src.namelist():
                    if name.startswith(('metallic_smoothness/','thickness/')):continue
                    data=src.read(name)
                    if name=='project.json':
                        meta=json.loads(data);meta['version']=2;meta['settings'].pop('material_hints');data=json.dumps(meta).encode()
                    dst.writestr(name,data)
            legacy=Project.load(old)
            self.assertEqual(legacy.frames[0].metallic_smoothness.getextrema(),(0,0))
            self.assertEqual(legacy.frames[0].thickness.getextrema(),(255,255))

    def test_failed_save_preserves_previous(self):
        p=Project([Frame('test',Image.new('RGBA',(2,2)))])
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'test.smproj';p.save(path);before=path.read_bytes()
            def fail(*args):raise RuntimeError('simulated disk failure')
            with self.assertRaises(RuntimeError):p.save(path,fail)
            self.assertEqual(path.read_bytes(),before)
            self.assertEqual(list(Path(d).iterdir()),[path])


if __name__=='__main__':unittest.main()
