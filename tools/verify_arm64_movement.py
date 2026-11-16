"""Compare the ARM64 movement serializer with the original full queued builder.

The x86 side includes slot-to-ID mapping and queue insertion. The ARM64 function
begins after that mapping; this does not claim an entire input subsystem port.
"""
import hashlib
import json
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
from inspect_film import parse
from verify_arm64_compat import load_arm64


def main():
    original = (ROOT / 'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    compiled = (ROOT / 'build/compat-arm64/libmyth181_compat.so').read_bytes()
    manifest = json.loads((ROOT / 'build/compat-arm64/build-manifest.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(compiled).hexdigest() == manifest['binary_sha256']
    for relative, digest in manifest['sources'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest
    pe = pefile.PE(data=original)
    cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    cpu.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    cpu.mem_write(base, pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000, 0x20000)
    arm, symbols = load_arm64(compiled)
    stop, stack = 0x1000000, 0x101f000
    slots, options_addr, points_addr, queue, queue_bytes, actors = 0x1002000, 0x1003000, 0x1004000, 0x1005000, 0x1006000, 0x1009000
    output, arm_opts, arm_ids, arm_points = 0x3009000, 0x300a000, 0x300b000, 0x300c000
    rng = random.Random(434)

    def arm_encode(options, ids, points, version, capacity=2048, override_points=None):
        arm.mem_write(output - 16, b'\xa5' * 2080)
        arm.mem_write(arm_opts, options)
        if ids:
            arm.mem_write(arm_ids, struct.pack('<' + 'h' * len(ids), *ids))
        if points:
            arm.mem_write(arm_points, b''.join(struct.pack('<ii', *point) for point in points))
        args = (output, capacity, arm_opts, arm_ids, len(ids), arm_points,
                len(points) if override_points is None else override_points, version)
        for index, value in enumerate(args):
            arm.reg_write(getattr(a, f'UC_ARM64_REG_X{index}'), value)
        arm.reg_write(a.UC_ARM64_REG_SP, 0x301f000)
        arm.reg_write(a.UC_ARM64_REG_LR, 0x3000000)
        arm.emu_start(symbols['myth181_encode_movement'], 0x3000000, count=100000)
        assert arm.reg_read(a.UC_ARM64_REG_PC) == 0x3000000
        assert arm.reg_read(a.UC_ARM64_REG_SP) == 0x301f000
        return arm.reg_read(a.UC_ARM64_REG_X0)

    def compare(options, ids, points, version):
        indices = rng.sample(range(140), len(ids))
        actor_bytes = bytearray(140 * 0xd8)
        for slot, stable in zip(indices, ids):
            struct.pack_into('<h', actor_bytes, slot * 0xd8, stable)
        cpu.mem_write(actors, bytes(actor_bytes))
        cpu.mem_write(0x529f54, struct.pack('<I', actors))
        cpu.mem_write(0x52a744, struct.pack('<I', version))
        cpu.mem_write(0x52f4e4, struct.pack('<I', queue))
        cpu.mem_write(queue, struct.pack('<7I', 0, 0, 0, 0, 0, 2048, queue_bytes))
        cpu.mem_write(queue_bytes - 16, b'\xa5' * 2080)
        if indices:
            cpu.mem_write(slots, struct.pack('<' + 'h' * len(indices), *indices))
        cpu.mem_write(options_addr, options)
        if points:
            cpu.mem_write(points_addr, b''.join(struct.pack('<ii', *point) for point in points))
        cpu.mem_write(stack, struct.pack('<6I', stop, 0xffffffff, options_addr, len(ids), len(points), points_addr))
        cpu.reg_write(x.UC_X86_REG_ESP, stack)
        cpu.reg_write(x.UC_X86_REG_ECX, slots)
        cpu.emu_start(0x446764, stop, count=100000)
        assert cpu.reg_read(x.UC_X86_REG_EIP) == stop
        assert cpu.reg_read(x.UC_X86_REG_ESP) == stack + 4
        size = struct.unpack('<I', cpu.mem_read(queue + 8, 4))[0]
        actual_size = arm_encode(options, ids, points, version)
        assert size == actual_size
        assert cpu.mem_read(queue_bytes - 16, 2080) == arm.mem_read(output - 16, 2080)
        assert arm.mem_read(output - 16, 16) == b'\xa5' * 16
        assert arm.mem_read(output + size, 2048 - size + 16) == b'\xa5' * (2048 - size + 16)
        return bytes(arm.mem_read(output, size))

    checks = 0
    for version in (1300, 1301, 2181):
        for count in (0, 1, 30, 31, 126, 127, 140):
            for point_count in (0, 1, 4):
                ids = rng.sample(range(1, 30000), count)
                points = [(rng.randrange(-0x100000, 0x100000), rng.randrange(-0x100000, 0x100000)) for _ in range(point_count)]
                compare(rng.randbytes(8), ids, points, version)
                checks += 1

    film = (ROOT / 'research/fixtures/films/capture-01.film').read_bytes()
    movement = next(c for c in parse(film)['commands'] if c['verb'] == 3)
    details = movement['movement_candidate']
    options = struct.pack('<3H2x', details['flags'], details['formation_index'], details['final_facing_raw'])
    points = [(p['raw_x'], p['raw_y']) for p in details['waypoints']]
    generated = bytearray(compare(options, details['unit_ids'], points, 2181))
    checks += 1
    generated[3] = movement['player']
    struct.pack_into('<I', generated, 4, movement['tick'])
    # Apply the original header converter after the known player/tick assignment.
    cpu.mem_write(queue_bytes, bytes(generated))
    cpu.mem_write(stack, struct.pack('<I', stop))
    cpu.reg_write(x.UC_X86_REG_ESP, stack)
    cpu.reg_write(x.UC_X86_REG_ECX, queue_bytes)
    cpu.emu_start(0x408b6a, stop, count=1000)
    expected_film = film[movement['file_offset']:movement['file_offset'] + movement['size']]
    assert bytes(cpu.mem_read(queue_bytes, len(generated))) == expected_film

    # Added boundary guards are tested separately, not claimed as original behavior.
    guard_checks = 0
    for capacity, point_count in ((0, None), (29, None), (2048, 4094), (2048, 0xffffffff)):
        result = arm_encode(options, details['unit_ids'], points, 2181, capacity, point_count)
        assert result == 0
        assert arm.mem_read(output - 16, 2080) == b'\xa5' * 2080
        guard_checks += 1
    report = dict(original_sha256=hashlib.sha256(original).hexdigest(), arm64_sha256=hashlib.sha256(compiled).hexdigest(),
                  original_builder_comparisons=checks, guard_checks=guard_checks, film_command_exact_match=True,
                  gameplay_versions=[1300,1301,2181], input_unit_counts=[0,1,30,31,126,127,140], waypoint_counts=[0,1,4],
                  limitations='Synthetic actor table. ARM64 covers serialization after stable-ID mapping, not original selection, scheduler, gameplay or input. Film player/tick assigned from the fixture. Malformed-input guard behavior is new, not original equivalence.')
    (ROOT / 'research/reports/arm64-movement-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'PASS: {checks} ARM64/original movement builds; Film exact match; {guard_checks} added guard checks.')


if __name__ == '__main__':
    main()
