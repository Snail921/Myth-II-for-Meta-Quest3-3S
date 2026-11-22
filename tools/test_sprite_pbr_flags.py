import json, tempfile, unittest, zipfile, struct
from pathlib import Path
from types import SimpleNamespace
from PIL import Image
from sprite_material import Frame, Project
from sprite_editor_commands import execute, revision, parse_frame_indices
from sprite_material_deploy import validate
from normal_library import material_frames,convert_packs,export_library,restore_library
class Flags(unittest.TestCase):
 def project(self):
  fs=[Frame(str(i),Image.new('RGBA',(2,2),(10+i,20,30,255))) for i in range(2)]
  for f in fs:f.metallic_smoothness.paste(153,(0,0,2,2));f.thickness.paste(77,(0,0,2,2))
  fs[1].pbr_enabled=True
  return Project(fs)
 def test_roundtrip_export_and_library(self):
  p=self.project()
  with tempfile.TemporaryDirectory() as d:
   r=Path(d);(r/'packs').mkdir();p.save(r/'a.smproj');q=Project.load(r/'a.smproj')
   self.assertEqual([f.pbr_enabled for f in q.frames],[False,True])
   for f in q.frames:self.assertEqual(f.metallic_smoothness.getpixel((0,0)),153)
   q.export_runtime(r/'packs/a.mvrnm');validate(r/'packs/a.mvrnm');rows=list(material_frames(r/'packs/a.mvrnm'))
   self.assertEqual([x[4] for x in rows],[0,1]);self.assertEqual(rows[0][3],q.normal(q.frames[0]).tobytes());self.assertEqual(rows[1][3],bytes([128,128,77,153])*4)
   convert_packs(r/'packs',r/'converted');export_library(r/'packs',r/'lib.zip');restore_library(r/'lib.zip',None,r/'restored')
   for folder in ['converted','restored']:self.assertEqual((r/folder/'a.mvrnm').read_bytes(),(r/'packs/a.mvrnm').read_bytes())
   data=bytearray((r/'packs/a.mvrnm').read_bytes());data[20]=2;(r/'bad.mvrnm').write_bytes(data)
   with self.assertRaises(ValueError):validate(r/'bad.mvrnm')
   with zipfile.ZipFile(r/'a.smproj') as z,zipfile.ZipFile(r/'old.smproj','w') as out:
    for name in z.namelist():
     b=z.read(name)
     if name=='project.json':
      meta=json.loads(b);meta['version']=3
      for f in meta['frames']:f.pop('pbr_enabled')
      b=json.dumps(meta).encode()
     out.writestr(name,b)
   self.assertFalse(any(f.pbr_enabled for f in Project.load(r/'old.smproj').frames))
 def test_api_atomic_undo_and_revision(self):
  p=self.project();e=SimpleNamespace(project=p,project_session='s',dirty=False,refresh=lambda:None);b=SimpleNamespace(editor=e)
  def call(**kw):
   body=dict(project_session='s',revision=revision(p),indices=[0,1],enabled=True);body.update(kw);return execute(b,'/pbr-set',body)
  before=revision(p);self.assertEqual(call(dry_run=True)[0],200);self.assertEqual(before,revision(p))
  for indices in [[0,99],[0,0],[True],[]]:self.assertEqual(call(indices=indices)[0],400);self.assertEqual(before,revision(p))
  self.assertEqual(call(enabled=1)[0],400);self.assertEqual(call()[0],200);self.assertNotEqual(before,revision(p));self.assertTrue(e.dirty)
  self.assertEqual(call(revision=before)[0],409)
  self.assertEqual(execute(b,'/material-undo',dict(project_session='s',revision=revision(p)))[0],200)
  self.assertEqual(revision(p),before);self.assertFalse(e.dirty)
  self.assertEqual(parse_frame_indices('0-1,1',2),[0,1]);self.assertEqual(parse_frame_indices('all',2),[0,1])
if __name__=='__main__':unittest.main()
