"""Execute compiled ARM64 kernels and original x86 routines on identical inputs.

No Android device or game execution. ELF loader deliberately accepts only the
small, relocation-free freestanding shared library produced by our build script.
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
from unicorn import x86_const as x
from unicorn import arm64_const as a


def load_arm64(data):
    assert data[:6] == b'\x7fELF\x02\x01', 'Expected ELF64 little endian'
    assert struct.unpack_from('<HH', data, 16) == (3, 183), 'Expected AArch64 shared object'
    phoff, shoff = struct.unpack_from('<QQ', data, 32)
    phsize, phnum, shsize, shnum = struct.unpack_from('<4H', data, 54)
    assert phsize == 56 and shsize == 64
    headers = [struct.unpack_from('<IIQQQQQQ', data, phoff + i * phsize) for i in range(phnum)]
    segments = [p for p in headers if p[0] == 1]
    assert segments and min(p[3] for p in segments) == 0
    size = (max(p[3] + p[6] for p in segments) + 4095) & ~4095
    assert size < 0x1000000
    arm = u.Uc(u.UC_ARCH_ARM64, u.UC_MODE_ARM)
    bias = 0x2000000
    arm.mem_map(bias, size)
    for _, _, offset, va, _, filesz, memsz, _ in segments:
        assert filesz <= memsz and offset + filesz <= len(data)
        arm.mem_write(bias + va, data[offset:offset + filesz])
    sections = [struct.unpack_from('<IIQQQQIIQQ', data, shoff + i * shsize) for i in range(shnum)]
    assert not any(s[1] in (4, 9) and s[5] for s in sections), 'Relocations are unsupported'
    symbols = {}
    for section in sections:
        if section[1] != 11:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        assert section[9] == 24
        for pos in range(section[4], section[4] + section[5], 24):
            name, info, _, index, value, _ = struct.unpack_from('<IBBHQQ', data, pos)
            if not name:
                continue
            name = names[name:names.index(b'\0', name)].decode('ascii')
            assert index != 0, f'Unresolved external: {name}'
            if info & 15 == 2:
                symbols[name] = bias + value
    arm.mem_map(0x3000000, 0x20000)
    arm.reg_write(a.UC_ARM64_REG_CPACR_EL1, 3 << 20)
    return arm, symbols


def main():
    original = (ROOT / 'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    compiled = (ROOT / 'build/compat-arm64/libmyth181_compat.so').read_bytes()
    manifest = json.loads((ROOT / 'build/compat-arm64/build-manifest.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(compiled).hexdigest() == manifest['binary_sha256'], 'Binary differs from build manifest'
    for relative, expected_hash in manifest['sources'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == expected_hash, 'Rebuild after source change'
    pe = pefile.PE(data=original)
    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000, 0x20000)
    arm, symbols = load_arm64(compiled)

    def invoke_x86(entry, eax=0, stack_arg=0, full=False):
        cpu.mem_write(0x101f000, struct.pack('<2I', 0x1000000, stack_arg))
        cpu.reg_write(x.UC_X86_REG_ESP, 0x101f000)
        cpu.reg_write(x.UC_X86_REG_EAX, eax)
        cpu.emu_start(entry, 0x1000000, count=10000)
        assert cpu.reg_read(x.UC_X86_REG_EIP) == 0x1000000
        assert cpu.reg_read(x.UC_X86_REG_ESP) == 0x101f004
        result = cpu.reg_read(x.UC_X86_REG_EAX)
        return result if full else result & 0xffff

    def invoke_arm(name, arg0, arg1=0, full=False):
        arm.reg_write(a.UC_ARM64_REG_SP, 0x301f000)
        arm.reg_write(a.UC_ARM64_REG_LR, 0x3000000)
        arm.reg_write(a.UC_ARM64_REG_X0, arg0)
        arm.reg_write(a.UC_ARM64_REG_X1, arg1)
        arm.emu_start(symbols[name], 0x3000000, count=10000)
        assert arm.reg_read(a.UC_ARM64_REG_PC) == 0x3000000
        assert arm.reg_read(a.UC_ARM64_REG_SP) == 0x301f000
        result = arm.reg_read(a.UC_ARM64_REG_X0)
        return result if full else result & 0xffff

    rng_checks = 0
    for seed in (0, 1, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xffffffff, 0xfded):
        invoke_x86(0x46bf56, seed)
        invoke_arm('myth181_seed_primary', 0x3003000, seed)
        assert cpu.mem_read(0x67383c, 4) == arm.mem_read(0x3003000, 4)
        cpu.mem_write(0x673840, struct.pack('<I', seed))
        arm.mem_write(0x3003004, struct.pack('<I', seed))
        for _ in range(4096):
            for original_entry, arm_state in ((0x46bf6a, 0x3003000), (0x46bf86, 0x3003004)):
                expected = invoke_x86(original_entry)
                actual = invoke_arm('myth181_random16', arm_state)
                assert actual == expected
                assert cpu.mem_read(0x67383c, 8) == arm.mem_read(0x3003000, 8)
                rng_checks += 1

    rng = random.Random(434)
    cases = [[v] * 9 for v in (0.0, -0.0, 1.0, -1.0, 1 / 512, -1 / 512, math.pi, -math.pi)]
    cases += [[rng.uniform(-1024, 1024) for _ in range(7)] + [rng.uniform(-8, 8) for _ in range(2)] for _ in range(2048)]
    cpu.reg_write(x.UC_X86_REG_FPCW, 0x37f)
    for values in cases:
        encoded = struct.pack('<9f', *values)
        cpu.mem_write(0x1001000, encoded)
        arm.mem_write(0x3001000, encoded)
        cpu.mem_write(0x1001ff0, b'\xa5' * 56)
        arm.mem_write(0x3001ff0, b'\xa5' * 56)
        cpu.reg_write(x.UC_X86_REG_ESI, 0x1001000)
        cpu.reg_write(x.UC_X86_REG_EDI, 0x1002000)
        invoke_x86(0x44ffb3)
        invoke_arm('myth181_pack_view_fields', 0x3001000, 0x3002000)
        assert cpu.mem_read(0x1001ff0, 56) == arm.mem_read(0x3001ff0, 56), values
        assert arm.mem_read(0x3001ff0, 16) == b'\xa5' * 16
        assert arm.mem_read(0x3002018, 16) == b'\xa5' * 16
    x_queue, a_queue, x_buffer, a_buffer = 0x1005000, 0x3005000, 0x1006000, 0x3006000
    x_command, a_command = 0x1008000, 0x3008000
    ring_counts = {'write': 0, 'read': 0, 'rejected': 0, 'wrapped': 0}

    def initialize_ring(capacity, read=0, write=0, wrap=0):
        state = [0, read, write, wrap, 0, capacity]
        cpu.mem_write(x_queue, struct.pack('<7I', *state, x_buffer))
        arm.mem_write(a_queue, struct.pack('<6IQ', *state, a_buffer))
        for machine, address in ((cpu, x_buffer), (arm, a_buffer)):
            machine.mem_write(address - 16, b'\xa5' * 16 + b'\xcc' * capacity + b'\xa5' * 16)

    def ring_step(capacity, command=None, max_tick=0):
        before = struct.unpack('<6I', cpu.mem_read(x_queue, 24))
        if command is not None:
            cpu.mem_write(x_command, command)
            arm.mem_write(a_command, command)
            cpu.reg_write(x.UC_X86_REG_ESI, x_queue)
            cpu.reg_write(x.UC_X86_REG_EDI, x_command)
            expected = invoke_x86(0x408d71, full=True)
            actual = invoke_arm('myth181_ring_write', a_queue, a_command, full=True)
            assert expected == actual
            ring_counts['write'] += 1
            ring_counts['rejected'] += expected == 0
        else:
            cpu.reg_write(x.UC_X86_REG_ECX, x_queue)
            expected = invoke_x86(0x408dd0, stack_arg=max_tick, full=True)
            actual = invoke_arm('myth181_ring_read', a_queue, max_tick, full=True)
            assert (None if expected == 0 else expected - x_buffer) == (None if actual == 0 else actual - a_buffer)
            ring_counts['read'] += 1
        assert cpu.mem_read(x_queue, 24) == arm.mem_read(a_queue, 24)
        after = struct.unpack('<6I', cpu.mem_read(x_queue, 24))
        if command is not None:
            ring_counts['wrapped'] += before[3] != after[3]
        assert struct.unpack('<I', cpu.mem_read(x_queue + 24, 4))[0] == x_buffer
        assert struct.unpack('<Q', arm.mem_read(a_queue + 24, 8))[0] == a_buffer
        assert cpu.mem_read(x_buffer - 16, capacity + 32) == arm.mem_read(a_buffer - 16, capacity + 32)
        assert arm.mem_read(a_buffer - 16, 16) == b'\xa5' * 16
        assert arm.mem_read(a_buffer + capacity, 16) == b'\xa5' * 16

    for read, write, size in ((0, 0, 64), (16, 48, 24), (32, 48, 24), (0, 48, 16)):
        initialize_ring(64, read, write, 64)
        ring_step(64, struct.pack('<hbbI', size, 3, 0, 154) + bytes(size - 8))
    rng = random.Random(434)
    for capacity in (64, 128, 2048):
        initialize_ring(capacity)
        tick = 0
        for _ in range(4000):
            tick += rng.randrange(3)
            if rng.random() < 0.58:
                size = rng.randrange(8, min(48, capacity) + 1)
                command = struct.pack('<hbbI', size, 3, 0, tick) + rng.randbytes(size - 8)
                ring_step(capacity, command)
            else:
                ring_step(capacity, max_tick=max(0, tick - rng.randrange(20)))

    report = dict(original_sha256=hashlib.sha256(original).hexdigest(), arm64_sha256=hashlib.sha256(compiled).hexdigest(),
                  rng_comparisons=rng_checks, view_comparisons=len(cases), symbols={k:hex(v) for k,v in symbols.items()},
                  command_ring=ring_counts,
                  result='pass', limitations='Unicorn execution of isolated original x86 and compiled ARM64 only. Not an Android loader/device test, complete engine, APK, VR rendering or whole-game determinism proof. View inputs restricted to finite sampled values.')
    (ROOT / 'research/reports/arm64-compat-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'PASS ARM64 vs original x86: {rng_checks} RNG + {len(cases)} view conversions + {ring_counts} ring operations.')


if __name__ == '__main__':
    main()
