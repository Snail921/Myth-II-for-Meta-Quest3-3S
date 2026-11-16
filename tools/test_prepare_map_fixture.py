"""Malformed-input checks for the original mesh fixture reader."""
from pathlib import Path
import struct
import unittest
from prepare_map_fixture import parse_mesh, schemas


class MapFixtureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw=(Path(__file__).resolve().parents[1]/'research/fixtures/maps/willow-creek/mesh.bin').read_bytes()
        cls.layouts=schemas()
        cls.valid=parse_mesh(cls.raw,cls.layouts)

    def changed(self, offset, value, fmt='>I'):
        data=bytearray(self.raw);struct.pack_into(fmt,data,offset,value)
        return bytes(data)

    def test_truncated_header(self):
        with self.assertRaises(ValueError):parse_mesh(self.raw[:1023],self.layouts)

    def test_truncated_declared_body(self):
        with self.assertRaises(ValueError):parse_mesh(self.raw[:-65],self.layouts)

    def test_uninterpreted_suffix_is_not_part_of_declared_body(self):
        parsed=parse_mesh(self.raw[:-64],self.layouts)
        self.assertEqual(parsed['trailing_bytes'],0)
        self.assertEqual(parsed['markers'],self.valid['markers'])

    def test_overlapping_sections(self):
        with self.assertRaises(ValueError):parse_mesh(self.changed(0x28,0),self.layouts)

    def test_bad_cell_count(self):
        with self.assertRaises(ValueError):parse_mesh(self.changed(0x10,12),self.layouts)

    def test_unknown_palette_reference(self):
        at=self.valid['sections']['markers']['offset']+6
        with self.assertRaises(ValueError):parse_mesh(self.changed(at,65535,'>H'),self.layouts)

    def test_bad_action_range(self):
        at=self.valid['sections']['map_actions']['offset']+24
        with self.assertRaises(ValueError):parse_mesh(self.changed(at,0xffffffff),self.layouts)

    def test_unknown_parameter_type(self):
        at=self.valid['sections']['map_actions']['offset']+64*self.valid['header']['map_action_count']
        with self.assertRaises(ValueError):parse_mesh(self.changed(at,21,'>H'),self.layouts)

    def test_raw_fourcc_high_byte_is_preserved(self):
        ids={p['marker_tag'] for p in self.valid['palette']}
        self.assertIn('pig\xff',ids)


if __name__=='__main__':unittest.main()
