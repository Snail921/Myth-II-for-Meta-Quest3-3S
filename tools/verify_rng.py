"""Compare a mathematical RNG model against original x86 routines in Unicorn.

Loads only the 76-byte routine block, no Windows game process or OS APIs.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-deps'))
import pefile
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP


def next_random(seed):
    state = (seed * 1664525 + 1013904223) & 0xffffffff
    return state, state >> 16


def main():
    path = ROOT / 'research/extracted/update-1.8.1/Myth II.exe'
    raw = path.read_bytes()
    pe = pefile.PE(data=raw)
    routines = pe.get_data(0x6bf56, 0x4c)
    expected = bytes.fromhex('85c0a33c386700750ac7053c386700edfd0000c3'
                             'a13c38670069c00d661900055ff36e3ca33c38670066a13e386700c3'
                             'a14038670069c00d661900055ff36e3ca34038670066a142386700c3')
    assert routines == expected, 'Binary differs from reviewed code; re-analyze first'
    uc = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    uc.mem_map(0x46b000, 0x1000)
    uc.mem_write(0x46bf56, routines)
    uc.mem_map(0x673000, 0x1000)
    uc.mem_map(0x1000000, 0x2000)
    stop, stack = 0x1001000, 0x1000ff0
    def invoke(entry, eax=0):
        uc.reg_write(UC_X86_REG_ESP, stack)
        uc.reg_write(UC_X86_REG_EAX, eax)
        uc.mem_write(stack, struct.pack('<I', stop))
        uc.emu_start(entry, stop, count=32)
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 4, 'Routine did not return normally'
        return uc.reg_read(UC_X86_REG_EAX) & 0xffff
    def read(addr):
        return struct.unpack('<I', uc.mem_read(addr, 4))[0]
    checks, vectors = 0, []
    for seed in (0, 1, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xffffffff, 0xfded):
        sentinel = 0x13579bdf
        uc.mem_write(0x673840, struct.pack('<I', sentinel))
        invoke(0x46bf56, seed)
        state = seed or 0xfded
        assert read(0x67383c) == state
        assert read(0x673840) == sentinel
        samples = []
        for i in range(4096):
            state, expected_value = next_random(state)
            value = invoke(0x46bf6a)
            assert value == expected_value and read(0x67383c) == state
            assert read(0x673840) == sentinel
            checks += 1
            if i < 4:
                samples.append(dict(state=state, value=value))
        # Secondary stream has no observed zero-seed normalization; initialize its memory directly.
        uc.mem_write(0x673840, struct.pack('<I', seed))
        primary_final = read(0x67383c)
        secondary = seed
        for i in range(4096):
            secondary, expected_value = next_random(secondary)
            value = invoke(0x46bf86)
            assert value == expected_value and read(0x673840) == secondary
            assert read(0x67383c) == primary_final
            checks += 1
        vectors.append(dict(input_seed=seed, primary_initial=seed or 0xfded,
                            primary_samples=samples, primary_final=state, secondary_final=secondary))
    report = dict(binary_sha256=hashlib.sha256(raw).hexdigest(), unicorn=unicorn.__version__,
                  original_code_hex=routines.hex(), comparisons=checks, vectors=vectors,
                  result='pass', limitations='Only isolated RNG routines tested. Caller order, initialization of secondary stream, whole-game determinism and ARM code generation are not tested.')
    (ROOT/'research/reports/rng-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'PASS: {checks} original-x86/model comparisons; both states checked for cross-stream interference.')


if __name__ == '__main__':
    main()
