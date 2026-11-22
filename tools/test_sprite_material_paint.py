import tempfile
import unittest
from pathlib import Path
import numpy as np
from PIL import Image
from sprite_material import Frame, Project
from sprite_material_paint import Stroke, transform, record


class PaintTests(unittest.TestCase):
    def project(self):
        im=Image.new('RGBA',(9,9),(120,160,80,255));im.putpixel((4,4),(255,255,255,0))
        return Project([Frame('tree',im),Frame('tree2',im.copy())])

    def test_blends_and_alpha(self):
        for mode,expected in [('Replace',.4),('Add',.9),('Sub',.1),('Screen',.7)]:
            p=self.project();f=p.frames[0];f.thickness=Image.new('L',(9,9),128)
            s=Stroke(f,'thickness',.4,3,.5,mode);s.point(3,4)
            self.assertAlmostEqual(f.thickness.getpixel((3,4))/255,expected,delta=.005)
            self.assertEqual(f.thickness.getpixel((4,4)),128)
            self.assertEqual(f.thickness.getpixel((8,8)),128)
            self.assertNotEqual(f.thickness.getpixel((5,4)),128)

    def test_brightness_clips_and_protects_alpha(self):
        p=self.project();f=p.frames[0]
        f.thickness=Image.new('L',(9,9),128)
        transform(p,[0],'thickness','brightness',brightness=.2)
        self.assertEqual(f.thickness.getpixel((1,1)),179)
        self.assertEqual(f.thickness.getpixel((4,4)),128)
        transform(p,[0],'thickness','brightness',brightness=1)
        self.assertEqual(f.thickness.getpixel((1,1)),255)
        transform(p,[0],'thickness','brightness',brightness=-1)
        self.assertEqual(f.thickness.getpixel((1,1)),0)
        self.assertEqual(f.thickness.getpixel((4,4)),128)

    def test_dense_path_is_stable(self):
        a=self.project().frames[0];b=self.project().frames[0]
        s=Stroke(a,'thickness',0,2,.8,'Replace');s.point(1,3);s.point(7,3)
        t=Stroke(b,'thickness',0,2,.8,'Replace')
        for x in range(1,8):t.point(x,3)
        np.testing.assert_array_equal(np.asarray(a.thickness),np.asarray(b.thickness))

    def test_range_transform_history_roundtrip(self):
        p=self.project();before=transform(p,[1],'thickness','copy_height')
        self.assertEqual(p.frames[0].thickness.getpixel((1,1)),255)
        self.assertEqual(p.frames[1].thickness.getpixel((4,4)),255)
        self.assertEqual(p.frames[1].thickness.getpixel((1,1)),p.height(p.frames[1]).getpixel((1,1)))
        self.assertTrue(record(p,'copy_height','thickness',before,origin='user'))
        transform(p,[1],'thickness','invert');transform(p,[1],'thickness','contrast',0)
        self.assertEqual(p.frames[1].thickness.getpixel((1,1)),128)
        self.assertEqual(p.frames[1].thickness.getpixel((4,4)),255)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'test.smproj';p.save(path);q=Project.load(path)
            self.assertEqual(q.history,p.history)
            self.assertEqual(q.frames[1].thickness.tobytes(),p.frames[1].thickness.tobytes())


if __name__=='__main__':unittest.main()
