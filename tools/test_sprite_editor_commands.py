import base64
import io
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from PIL import Image
from sprite_material import Frame, Project
from sprite_editor_commands import execute, revision


class CommandsTests(unittest.TestCase):
    def setUp(self):
        image = Image.new('RGBA', (3, 1))
        image.putdata([(10, 20, 30, 255), (20, 20, 30, 255), (10, 20, 30, 0)])
        p = Project([Frame('test', image)])
        p.material_hints[0]['colors'][0] = [10, 20, 30]
        self.e = SimpleNamespace(project=p, project_session='session', dirty=True,
                                 refresh=lambda: None, view=SimpleNamespace(set=lambda _: None))
        self.b = SimpleNamespace(editor=self.e)

    def call(self, route='/material-edit', **extra):
        im = Image.new('L', (3, 1)); im.putdata([255,0,255])
        out = io.BytesIO(); im.save(out, format='PNG')
        row = dict(index=0, image_sha256=self.e.project.frames[0].identity['image_sha256'],
                   png_base64=base64.b64encode(out.getvalue()).decode())
        body = dict(project_session='session', revision=revision(self.e.project),
                    channel='metallic_smoothness', value=.6, masks=[row])
        body.update(extra)
        status, _, data = execute(self.b, route, body)
        return status, json.loads(data)

    def test_live_edit_preserves_dirty_work_and_undo(self):
        self.e.project.frames[0].mask.putpixel((1, 0), 150)
        before = revision(self.e.project)
        result = self.call(dry_run=True, preview_index=0)[1]
        self.assertEqual(result['pixels'], 1)
        self.assertTrue(result['preview_png_base64'])
        self.assertEqual(self.call(hint_index=0)[0], 400)
        self.assertEqual(revision(self.e.project), before)
        self.assertEqual(self.call()[0], 200)
        f = self.e.project.frames[0]
        self.assertEqual(list(f.metallic_smoothness.getdata()), [153, 0, 0])
        self.assertEqual(f.mask.getpixel((1, 0)), 150)
        self.assertEqual(list(f.thickness.getdata()), [255]*3)
        self.assertEqual(self.call('/material-undo')[0], 200)
        self.assertEqual(revision(self.e.project), before)
        self.assertTrue(self.e.dirty)

    def test_stale_revision_and_undo_do_not_overwrite_user_work(self):
        old = revision(self.e.project)
        self.call()
        self.assertEqual(self.call(revision=old)[0], 409)
        self.e.project.frames[0].mask.putpixel((0, 0), 255)
        self.assertEqual(self.call('/material-undo')[0], 409)
        self.assertEqual(self.call(project_session='old')[0], 409)

    def test_invalid_mask_batch_is_atomic(self):
        im = Image.new('L', (3, 1), 255)
        out = io.BytesIO(); im.save(out, format='PNG')
        row = dict(index=0, image_sha256=self.e.project.frames[0].identity['image_sha256'],
                   png_base64=base64.b64encode(out.getvalue()).decode())
        before = revision(self.e.project)
        body = dict(project_session='session', revision=before, channel='thickness', value=.3,
                    masks=[row, dict(row, index=999)])
        self.assertEqual(execute(self.b, '/material-edit', body)[0], 400)
        self.assertEqual(revision(self.e.project), before)
        body['masks'] = [row]
        self.assertEqual(execute(self.b, '/material-edit', body)[0], 200)
        self.assertEqual(list(self.e.project.frames[0].thickness.getdata()), [76, 76, 255])

    def test_save_live_project_without_dialog_and_guard_load(self):
        self.e.task = lambda title, work, done: done(work(lambda *args: None))
        with tempfile.TemporaryDirectory() as folder:
            path = str(Path(folder) / 'live.smproj')
            self.assertEqual(self.call('/load-project', path=path)[0], 409)
            self.assertEqual(self.call('/save-project', path=path)[0], 202)
            self.assertFalse(self.e.dirty)
            self.assertEqual(self.b.last_operation['status'], 'complete')
            self.assertEqual(Project.load(path).material_hints, self.e.project.material_hints)
            self.assertEqual(self.call('/save-project', path=path)[0], 409)
            self.assertEqual(self.call('/save-project', path=path, overwrite=True)[0], 202)


if __name__ == '__main__':
    unittest.main()
