"""Check original nine-float to 24-byte view-field conversion, including x87 helper.

Field meanings remain hypotheses; scales and storage widths are verified here.
"""
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-deps'))
import pefile
import unicorn as u
from unicorn.x86_const import UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW


def encode_model(values, angular_scale):
    # Round input exactly once to the float32 values read by the original.
    values = struct.unpack('<9f', struct.pack('<9f', *values))
    integers = [math.trunc(v * (512.0 if i < 7 else angular_scale)) for i, v in enumerate(values)]
    return struct.pack('<3i6H', *integers[:3], *(value & 0xffff for value in integers[3:]))


def main():
    data = (ROOT / 'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    angular_scale = struct.unpack('<f', pe.get_data(0x522f54 - base, 4))[0]
    assert struct.unpack('<f', pe.get_data(0x522f58 - base, 4))[0] == 512.0
    uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    uc.mem_write(base, pe.get_memory_mapped_image())
    uc.mem_map(0x1000000, 0x20000)
    stop, source, output, stack = 0x1000000, 0x1001000, 0x1002000, 0x101f000
    rng = random.Random(434)
    cases = [[v] * 9 for v in (0.0, -0.0, 1.0, -1.0, 1 / 512, -1 / 512, math.pi, -math.pi)]
    cases += [[rng.uniform(-1024, 1024) for _ in range(7)] + [rng.uniform(-8, 8) for _ in range(2)] for _ in range(2048)]
    for values in cases:
        uc.mem_write(source, struct.pack('<9f', *values))
        uc.mem_write(output - 16, b'\xa5' * 56)
        uc.mem_write(stack, struct.pack('<I', stop))
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.reg_write(UC_X86_REG_ESI, source)
        uc.reg_write(UC_X86_REG_EDI, output)
        uc.reg_write(UC_X86_REG_FPCW, 0x37f)
        uc.emu_start(0x44ffb3, stop, count=10000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 4
        actual = bytes(uc.mem_read(output, 24))
        expected = encode_model(values, angular_scale)
        assert actual == expected, (values, actual.hex(), expected.hex())
        assert bytes(uc.mem_read(output - 16, 16)) == b'\xa5' * 16
        assert bytes(uc.mem_read(output + 24, 16)) == b'\xa5' * 16
    report = dict(binary_sha256=hashlib.sha256(data).hexdigest(), comparisons=len(cases),
                  source_float_count=9, output_bytes=24, linear_scale=512.0, angular_scale_float32=angular_scale,
                  output_layout='three signed int32 followed by six low-16-bit words, native little endian',
                  limitations='Finite sampled float32 inputs, x87 control word 0x37f only. No NaN/infinity, int64 overflow, runtime camera input, projection or full-field semantic validation. No original function was stubbed.')
    (ROOT / 'research/reports/view-encoding-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'PASS: {len(cases)} original view conversions matched, including original x87 conversion helper.')


if __name__ == '__main__':
    main()
