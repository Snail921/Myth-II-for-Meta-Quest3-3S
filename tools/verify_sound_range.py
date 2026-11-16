"""Read-only Build 434 attenuation oracle; no game/runtime files are patched."""
import struct
from verify_combat_builders import machine, run, STOP
from unicorn.x86_const import *

SOUND = STOP + 0x20000
for category in range(4):
    c = machine()
    c.mem_write(0x52db14, struct.pack('<I', SOUND))
    c.mem_write(SOUND, struct.pack('<Ih', 0, category))
    curves = struct.unpack('<hhIIhhII', c.mem_read(0x52e718 + category * 24, 24))
    for underwater in (0, 1):
        near, far, start, end = curves[:4] if underwater else curves[4:]
        for distance in (0, 4096, 8192, 12288, 18432, 24576, 32767):
            run(c, 0x4cc71a, [underwater], {UC_X86_REG_EAX: 0, UC_X86_REG_EDI: distance})
            expected = near if distance <= start else far if distance > end else far + (near-far)*(end-distance)//(end-start)
            assert c.reg_read(UC_X86_REG_EAX) == expected
    values = []
    for factor in (1, 1.5, 2):
        # Scale the true distance BEFORE the original 32767 clamp.
        distance = min(32767, int(30 * 512 / factor))
        run(c, 0x4cc71a, [0], {UC_X86_REG_EAX: 0, UC_X86_REG_EDI: distance})
        values.append(c.reg_read(UC_X86_REG_EAX))
    print(f'loudness={category} normal={curves[4:]} gain@30units for 1x/1.5x/2x={values}')

# Actual initial-play branch skips allocation if effective gain is zero.
for gain, stop in [(0, 0x4cbc04), (128, 0x4cbba1)]:
    c = machine()
    frame = STOP + 0x21000
    c.mem_write(frame - 0x2a, struct.pack('<h', gain))
    c.reg_write(UC_X86_REG_EBP, frame)
    c.emu_start(0x4cbb9a, stop, count=10)
    assert c.reg_read(UC_X86_REG_EIP) == stop
print('PASS original attenuation curves and zero-volume pre-play exclusion')
