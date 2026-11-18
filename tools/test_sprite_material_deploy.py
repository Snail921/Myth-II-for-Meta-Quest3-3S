import hashlib
import shlex
import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path
from sprite_material_deploy import deploy, REMOTE, validate, LIMIT

PACK = b'MVRNORM1' + struct.pack('<III', 1, 1, 1) + bytes(8)


class Device:
    def __init__(self, fail=''):
        self.files = {REMOTE+'/warrior.mvrnm': b'old', REMOTE+'/other.mvrnm': PACK}
        self.fail = fail
        self.logs = []
        self.log = self.logs.append
        self.checks = 0

    def ready(self):
        self.checks += 1

    def exists(self, p):
        return p in self.files

    def call(self, *args, timeout=None):
        if args[0] == 'push':
            self.files[args[2]] = Path(args[1]).read_bytes()
            return ''
        t = shlex.split(args[1])
        if t[0] == 'df':
            return 'Filesystem Size Used Available Use Mounted\n/dev/fuse 999999999 0 999999999 0 /sdcard'
        if t[0] == 'find':
            return '\n'.join(self.files)
        if t[0] == 'stat':
            return str(len(self.files[t[-1]]))
        if t[0] == 'sha256sum':
            return ('bad' if self.fail == 'hash' else hashlib.sha256(self.files[t[1]]).hexdigest()) + '  file'
        if t[0] == 'mv':
            if self.fail == 'publish' and '/.incoming-' in t[1] and t[2].endswith('/warrior.mvrnm'):
                self.fail = ''
                raise RuntimeError('publish failed')
            self.files[t[2]] = self.files.pop(t[1])
        if t[0] == 'rm':
            self.files.pop(t[-1], None)
        return ''


class Tests(unittest.TestCase):
    def test_transfer_replaces_without_retaining_old_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'warrior.mvrnm').write_bytes(PACK)
            (root/"new unit's.MVRNM").write_bytes(PACK)
            (root/'ignore.png').write_bytes(b'ignore')
            nested = root/'nested'; nested.mkdir(); (nested/'skip.mvrnm').write_bytes(PACK)
            d = Device(); deploy(root, d)
            self.assertEqual(d.files[REMOTE+'/warrior.mvrnm'], PACK)
            self.assertEqual(d.files[REMOTE+"/new unit's.mvrnm"], PACK)
            self.assertEqual(d.files[REMOTE+'/other.mvrnm'], PACK)
            self.assertFalse(any('/.backup-' in p for p in d.files))
            self.assertFalse(any('/.incoming-' in p or 'skip.mvrnm' in p for p in d.files))
            self.assertEqual(d.checks, 2)
            self.assertTrue(any('"current": 2' in s for s in d.logs))

    def test_failures_preserve_existing(self):
        for failure in ('hash', 'publish'):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                (root/'a.mvrnm').write_bytes(PACK)
                (root/'warrior.mvrnm').write_bytes(PACK)
                d = Device(failure); before = dict(d.files)
                with self.assertRaises((ValueError, RuntimeError)):
                    deploy(root, d)
                self.assertEqual(d.files, before)

    def test_combined_library_above_64_mib_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ('a.mvrnm', 'b.mvrnm'):(root/name).write_bytes(PACK)
            d = Device()
            with patch('sprite_material_deploy.validate', return_value=40*1024*1024):
                deploy(root, d)
            self.assertEqual(d.files[REMOTE+'/a.mvrnm'], PACK)
            self.assertEqual(d.files[REMOTE+'/b.mvrnm'], PACK)

    def test_invalid_and_empty(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); d = Device()
            with self.assertRaises(ValueError): deploy(root, d)
            p = root/'bad.mvrnm'
            for data in (PACK[:-1], PACK+b'x', b'wrong header', b'MVRNORM1'+struct.pack('<I', 0)):
                p.write_bytes(data)
                with self.assertRaises(ValueError): validate(p)
            self.assertEqual(d.checks, 0)


if __name__ == '__main__':
    unittest.main()
