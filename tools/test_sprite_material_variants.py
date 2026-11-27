import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from PIL import Image
from sprite_material import Frame, Project
from sprite_material_variants import duplicate, replace_albedo, copy_materials
from sprite_material_deploy import validate
from normal_library import convert_packs, export_library, restore_library
from sprite_editor_commands import execute, revision


class Variants(unittest.TestCase):
    def test_recovery_can_switch_variant_but_explicit_saves_cannot(self):
        p=self.source();q=duplicate(p)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'sprite-editor-last-session.smproj'
            p.save(path,recovery={'dirty':True})
            with self.assertRaises(ValueError):q.save(path)
            q.save(path,recovery={'dirty':True})
            self.assertEqual(Project.load(path).variant,'unshadowed')
            p.save(path,recovery={'dirty':False})
            self.assertEqual(Project.load(path).variant,'shadowed')
            ordinary=Path(d)/'original.smproj';p.save(ordinary)
            with self.assertRaises(ValueError):q.save(ordinary,recovery={})
            path.unlink();p.save(path)
            with self.assertRaises(ValueError):q.save(path,recovery={})

    def test_per_image_normal_inversion(self):
        p=self.source();p.frames.append(Frame('other',p.frames[0].image.copy()))
        f=p.frames[0];f.normal_ink=Image.new('RGBA',f.image.size,(90,170,230,255))
        original=p.normal(f);other=p.normal(p.frames[1]);f.normal_inverted=True
        result=p.normal(f)
        self.assertEqual(result.getpixel((0,0)),(165,85,230,255))
        self.assertEqual(other.tobytes(),p.normal(p.frames[1]).tobytes())
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'test.smproj';p.save(path);loaded=Project.load(path)
            self.assertTrue(loaded.frames[0].normal_inverted)
            self.assertEqual(loaded.normal(loaded.frames[0]).tobytes(),result.tobytes())
            loaded.export_runtime(Path(d)/'test.mvrnm')
            from normal_library import material_frames
            self.assertEqual(list(material_frames(Path(d)/'test.mvrnm'))[0][3],result.tobytes())
        q=duplicate(p);q.frames[0].normal_inverted=True;copy_materials(q,p)
        self.assertFalse(q.frames[0].normal_inverted)
        self.assertEqual(q.normal(q.frames[0]).tobytes(),result.tobytes())
        f.normal_inverted=False;self.assertEqual(p.normal(f).tobytes(),original.tobytes())

    def source(self):
        return Project([Frame('test', Image.new('RGBA', (2,2), (40,60,80,255)))], 'fixture')

    def test_roundtrip_and_protection(self):
        p=self.source(); q=duplicate(p)
        replace_albedo(q,0,Image.new('RGBA',(2,2),(80,100,120,255)))
        self.assertEqual(p.frames[0].image.getpixel((0,0)),(40,60,80,255))
        with tempfile.TemporaryDirectory() as d:
            root=Path(d); (root/'packs').mkdir()
            p.save(root/'original.smproj'); q.save(root/'variant.smproj')
            loaded=Project.load(root/'variant.smproj')
            self.assertEqual(p.source_collection_id,loaded.source_collection_id)
            self.assertEqual(loaded.frames[0].image.tobytes(),q.frames[0].image.tobytes())
            for project,path in [(q,root/'original.smproj'),(p,root/'variant.smproj')]:
                with self.assertRaises(ValueError):project.save(path)
            p.export_runtime(root/'packs/original.mvrnm'); q.export_runtime(root/'packs/variant.mvrnm')
            with self.assertRaises(ValueError):q.export_runtime(root/'packs/original.mvrnm')
            validate(root/'packs/variant.mvrnm')
            from unittest.mock import patch
            from sprite_runtime_import import import_pack
            with patch.object(Project,'collection',side_effect=lambda *args:self.source()):
                imported=import_pack(root/'packs/variant.mvrnm','original.bin')
            self.assertEqual(imported.variant,'unshadowed')
            self.assertEqual(imported.frames[0].image.tobytes(),q.frames[0].image.tobytes())
            imported.export_runtime(root/'roundtrip.mvrnm')
            self.assertEqual((root/'roundtrip.mvrnm').read_bytes(),(root/'packs/variant.mvrnm').read_bytes())
            convert_packs(root/'packs',root/'converted')
            export_library(root/'packs',root/'lib.zip'); restore_library(root/'lib.zip',None,root/'restored')
            for folder in ['converted','restored']:
                self.assertEqual((root/folder/'variant.mvrnm').read_bytes(),(root/'packs/variant.mvrnm').read_bytes())
            corrupt=bytearray((root/'packs/variant.mvrnm').read_bytes()); corrupt[-1]=0
            (root/'bad.mvrnm').write_bytes(corrupt)
            with self.assertRaises(ValueError):validate(root/'bad.mvrnm')

    def test_copy_evaluated_maps_and_independent_flags(self):
        p=self.source();q=duplicate(p)
        replace_albedo(q,0,Image.new('RGBA',(2,2),(100,120,140,255)))
        q.frames[0].pbr_enabled=True
        copy_materials(p,q)
        self.assertEqual(p.height(p.frames[0]).tobytes(),q.height(q.frames[0]).tobytes())
        self.assertEqual(p.normal(p.frames[0]).tobytes(),q.normal(q.frames[0]).tobytes())
        self.assertFalse(p.frames[0].pbr_enabled)
        with self.assertRaises(ValueError):replace_albedo(q,0,Image.new('RGBA',(2,2),(0,0,0,0)))
        other=Project([Frame('test',Image.new('RGBA',(2,2),(1,2,3,255)))])
        with self.assertRaises(ValueError):copy_materials(p,other)

    def test_api_session_and_dirty_protection(self):
        e=SimpleNamespace(project=self.source(),project_session='s',dirty=True,refresh=lambda:None)
        e.accept=lambda p:setattr(e,'project',p)
        e.task=lambda title,work,done:done(work(lambda *args:None))
        bridge=SimpleNamespace(editor=e)
        with tempfile.TemporaryDirectory() as d:
            body=dict(project_session='s',revision=revision(e.project),path=str(Path(d)/'new.smproj'))
            self.assertEqual(execute(bridge,'/duplicate-unshadowed',body)[0],409)
            e.dirty=False
            self.assertEqual(execute(bridge,'/duplicate-unshadowed',dict(body,revision='stale'))[0],409)
            self.assertEqual(execute(bridge,'/duplicate-unshadowed',body)[0],202)
            self.assertEqual(e.project.variant,'unshadowed')
            body['revision']=revision(e.project)
            self.assertEqual(execute(bridge,'/duplicate-unshadowed',body)[0],400)


if __name__=='__main__':unittest.main()
