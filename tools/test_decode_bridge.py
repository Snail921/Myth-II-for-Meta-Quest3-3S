"""Regression checks against a real paused snapshot and malformed IPC payloads."""
import struct
import unittest
from pathlib import Path
from decode_bridge import decode

DATA = (Path(__file__).resolve().parents[1] / 'research/fixtures/bridge/paused-0000.bin').read_bytes()

class BridgeTests(unittest.TestCase):
    def test_known_original_unit(self):
        scene = decode(DATA)
        self.assertEqual(scene['tick'], 4560)
        self.assertEqual(len(scene['instances']), 1323)
        unit = next(x for x in scene['instances'] if x.get('unit_id') == 10359)
        self.assertEqual(unit['position_raw'], [95838, 80040, -932])
        self.assertEqual(unit['collection_index'], 21)
        self.assertEqual(sum('unit_id' in x for x in scene['instances']), 80)

    def test_truncated_and_extra_payload(self):
        for data in (DATA[:63], DATA[:-1], DATA + b'\0'):
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                decode(data)

    def test_corrupt_headers(self):
        for offset, value in ((8, 2), (16, 4561), (24, 401), (28, 8193), (32, 215), (36, 79)):
            data = bytearray(DATA)
            struct.pack_into('<I', data, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                decode(data)

    def test_invalid_actor_reference(self):
        data = bytearray(DATA)
        struct.pack_into('<h', data, 64 + 12, 1323)
        with self.assertRaises(ValueError):
            decode(data)

    def test_selection_metadata_v2(self):
        data=bytearray(DATA)
        struct.pack_into('<I',data,8,2)
        data.extend(struct.pack('<iiIII126h',0,0,3,0,1,0,*([0]*125)))
        scene=decode(data)
        unit=next(x for x in scene['instances'] if x.get('unit_id')==10359)
        self.assertTrue(unit['selected'])
        self.assertTrue(unit['controllable'])
        self.assertEqual(unit['team_raw'],0)
        self.assertEqual(unit['owner_raw'],0)
        for offset,value in ((len(DATA)+16,127),(len(DATA)+20,400)):
            malformed=bytearray(data)
            struct.pack_into('<I',malformed,offset,value)
            with self.assertRaises(ValueError):decode(malformed)

if __name__ == '__main__':
    unittest.main()
