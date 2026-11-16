"""Differentially check recovered command ring behavior against original x86.

Synthetic queue states only; this is not a game simulation or replay test.
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
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP,
)


def model_write(state, memory, command):
    size, _, _, tick = struct.unpack('<hbbI', command[:8])
    state[4] = max(state[4], tick)
    read, write, capacity = state[1], state[2], state[5]
    if write >= read and write + size > capacity:
        state[2], state[3] = 0, write
    write = state[2]
    free = read - write if read > write else capacity - write + read
    if free <= size:
        return 0
    memory[write:write + size] = command
    state[2] += size
    return 1


def model_read(state, memory, max_tick):
    if state[1] > state[2] and state[1] >= state[3]:
        state[1] = 0
    offset = state[1]
    if offset != state[2]:
        size, _, _, tick = struct.unpack('<hbbI', memory[offset:offset + 8])
        if tick <= max_tick:
            state[1] += size
            return state[6] + offset
    return 0


def main():
    binary = (ROOT / 'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    pe = pefile.PE(data=binary)
    uc = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
    base = pe.OPTIONAL_HEADER.ImageBase
    uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    uc.mem_write(base, pe.get_memory_mapped_image())
    uc.mem_map(0x1000000, 0x20000)
    stop, queue, buffer, command_addr, stack = 0x1000000, 0x1001000, 0x1002000, 0x1004000, 0x101f000
    rng = random.Random(434)
    counts = {'write': 0, 'read': 0, 'rejected': 0, 'wrapped': 0}
    examples = []

    def check(operation, state, memory, command=None, max_tick=0):
        before = state[:]
        uc.mem_write(queue, struct.pack('<7I', *state))
        uc.mem_write(buffer - 16, b'\xa5' * 16 + bytes(memory) + b'\xa5' * 16)
        uc.mem_write(stack, struct.pack('<2I', stop, max_tick))
        uc.reg_write(UC_X86_REG_ESP, stack)
        if operation == 'write':
            uc.mem_write(command_addr, command)
            uc.reg_write(UC_X86_REG_ESI, queue)
            uc.reg_write(UC_X86_REG_EDI, command_addr)
            expected = model_write(state, memory, command)
            entry = 0x408d71
        else:
            uc.reg_write(UC_X86_REG_ECX, queue)
            expected = model_read(state, memory, max_tick)
            entry = 0x408dd0
        uc.emu_start(entry, stop, count=10000)
        assert uc.reg_read(UC_X86_REG_EIP) == stop
        assert uc.reg_read(UC_X86_REG_ESP) == stack + 4
        assert uc.reg_read(UC_X86_REG_EAX) == expected, (operation, before)
        assert list(struct.unpack('<7I', uc.mem_read(queue, 28))) == state
        assert bytes(uc.mem_read(buffer, len(memory))) == memory
        assert bytes(uc.mem_read(buffer - 16, 16)) == b'\xa5' * 16
        assert bytes(uc.mem_read(buffer + len(memory), 16)) == b'\xa5' * 16
        counts[operation] += 1
        if operation == 'write':
            counts['rejected'] += expected == 0
            counts['wrapped'] += state[3] != before[3]
        return expected

    # Independent boundary states: exact-fit rejection and wrap-on-failed-write.
    for read, write, size in [(0, 0, 64), (16, 48, 24), (32, 48, 24), (0, 48, 16)]:
        state = [0, read, write, 64, 0, 64, buffer]
        memory = bytearray([0xcc] * 64)
        command = struct.pack('<hbbI', size, 3, 0, 154) + bytes(size - 8)
        before = state[:]
        result = check('write', state, memory, command)
        examples.append({'before': before[:6], 'size': size, 'result': result, 'after': state[:6]})

    # Operation sequences exercise both routines on states produced by the original.
    for capacity in (64, 128, 2048):
        state = [0, 0, 0, 0, 0, capacity, buffer]
        memory = bytearray([0xcc] * capacity)
        tick = 0
        for _ in range(4000):
            tick += rng.randrange(3)
            if rng.random() < 0.58:
                size = rng.randrange(8, min(48, capacity) + 1)
                command = struct.pack('<hbbI', size, 3, 0, tick) + rng.randbytes(size - 8)
                check('write', state, memory, command)
            else:
                check('read', state, memory, max_tick=max(0, tick - rng.randrange(20)))
    report = dict(binary_sha256=hashlib.sha256(binary).hexdigest(), seed=434,
                  counts=counts, boundary_examples=examples,
                  limitations='Nonnegative signed-range ticks and valid positive command sizes only. Synthetic ring states; no gameplay, threading, replay or networking equivalence established.')
    (ROOT / 'research/reports/command-ring-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(counts))


if __name__ == '__main__':
    main()
