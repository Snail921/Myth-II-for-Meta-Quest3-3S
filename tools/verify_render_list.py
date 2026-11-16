"""Probe original render-list linkage in a narrow, explicitly synthetic branch."""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-deps'))
import pefile
import unicorn as u
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP


def main():
    data = (ROOT / 'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    pe = pefile.PE(data=data)
    uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    uc.mem_write(base, pe.get_memory_mapped_image())
    uc.mem_map(0x1000000, 0x20000)
    stop, view, linked, nodes, stack = 0x1000000, 0x1001000, 0x1002000, 0x1003000, 0x101f000

    def run(entry, eax):
        uc.mem_write(stack, struct.pack('<I', stop))
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.reg_write(UC_X86_REG_EAX, eax)
        uc.reg_write(UC_X86_REG_ESI, view)
        uc.emu_start(entry, stop, count=10000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 4

    cases = []
    for view_kind in (4, 5):
        for override, flags in ((0, 0), (123456, 0), (0, 0x200)):
            uc.mem_write(linked - 16, b'\xa5' * 48)
            run(0x4c7ca0, linked)
            assert bytes(uc.mem_read(linked, 16)) == bytes(16)
            uc.mem_write(view, bytes(0xac))
            uc.mem_write(view + 0x42, struct.pack('<h', view_kind))
            uc.mem_write(view + 0xa0, struct.pack('<I', linked))
            uc.mem_write(linked + 12, struct.pack('<I', override))
            for index in range(32):
                node = nodes + index * 0x40
                initial = bytearray([0xcc] * 0x40)
                initial[8:12] = struct.pack('<I', 0x1234)
                initial[16:20] = struct.pack('<I', index + 1)
                initial[22:24] = struct.pack('<H', flags)
                uc.mem_write(node, bytes(initial))
                run(0x4c7cb0, node)
                first, last, count, key = struct.unpack('<4I', uc.mem_read(linked, 16))
                assert (first, last, count, key) == (nodes, node, index + 1, override)
                for j in range(index + 1):
                    current = nodes + j * 0x40
                    prev, following, sort_key = struct.unpack('<3I', uc.mem_read(current, 12))
                    assert prev == (current - 0x40 if j else 0)
                    assert following == (current + 0x40 if j < index else 0)
                    expected_key = override or (0x1234 if flags & 0x200 else (j + 1) << 14)
                    assert sort_key == expected_key
                assert bytes(uc.mem_read(linked - 16, 16)) == b'\xa5' * 16
                assert bytes(uc.mem_read(linked + 16, 16)) == b'\xa5' * 16
            cases.append(dict(view_kind=view_kind, override=override, flags=flags, nodes=32))
    report = dict(binary_sha256=hashlib.sha256(data).hexdigest(), initializations=6, appends=192, cases=cases,
                  limitations='Only view kinds 4/5 and flags without 0xe000, bypassing 0x4b9ca5 by original branch logic. No hooks/stubs. Does not validate visibility, allocation, other view modes, rendering or stereo behavior.')
    (ROOT / 'research/reports/render-list-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Verified 6 initializations and 192 appends against original x86.')


if __name__ == '__main__':
    main()
