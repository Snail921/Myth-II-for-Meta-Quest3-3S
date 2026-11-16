"""Regression cases for the captured real Film and corrupt framing."""
from pathlib import Path
import struct
import unittest
from inspect_film import parse

FIXTURE = Path(__file__).resolve().parents[1]/'research/fixtures/films/capture-01.film'


class FilmTests(unittest.TestCase):
    def setUp(self):
        self.raw = FIXTURE.read_bytes()

    def test_real_recording_zero_size(self):
        result = parse(self.raw)
        self.assertTrue(result['size_from_eof'])
        self.assertEqual(result['ending_tick'],263)
        self.assertEqual([c['verb'] for c in result['commands']],[16,3,1])
        self.assertEqual([c['tick'] for c in result['commands']],[6,154,171])
        self.assertTrue(result['command_stream_roundtrip_equal'])
        self.assertTrue(result['terminal_block_seen'])
        self.assertEqual(result['trailing_payload_bytes'],0)

    def test_explicit_payload_length(self):
        raw = bytearray(self.raw)
        struct.pack_into('>i',raw,48,len(raw)-64)
        self.assertEqual(parse(raw)['command_count'],3)

    def test_truncated_payload(self):
        with self.assertRaises(ValueError):
            parse(self.raw[:-1])

    def test_zero_command_size(self):
        raw = bytearray(self.raw)
        struct.pack_into('>h',raw,2672,0)
        with self.assertRaises(ValueError):
            parse(raw)

    def test_wrong_command_count(self):
        raw = bytearray(self.raw)
        struct.pack_into('>h',raw,2670,4)
        with self.assertRaises(ValueError):
            parse(raw)

    def test_oversized_block(self):
        raw = bytearray(self.raw)
        struct.pack_into('>h',raw,2668,2049)
        with self.assertRaises(ValueError):
            parse(raw)

    def test_invalid_movement_count_preserves_raw_payload(self):
        raw = bytearray(self.raw)
        struct.pack_into('>h',raw,2736,32767)
        command = parse(raw)['commands'][1]
        self.assertIn('payload_decode_error',command)
        self.assertEqual(len(bytes.fromhex(command['payload_hex'])),22)


if __name__ == '__main__':
    unittest.main()
