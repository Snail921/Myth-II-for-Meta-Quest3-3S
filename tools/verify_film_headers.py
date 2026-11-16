"""Compare parsed real Film headers with the original EXE's x86 conversion routine."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/python-deps'))
import pefile
import unicorn as u
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from inspect_film import parse


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('film', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    binary = (ROOT/'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    pe = pefile.PE(data=binary)
    raw = args.film.read_bytes()
    parsed = parse(raw)
    base = pe.OPTIONAL_HEADER.ImageBase
    uc = u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
    uc.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
    uc.mem_write(base,pe.get_memory_mapped_image())
    uc.mem_map(0x1000000,0x3000)
    stack, stop, buf = 0x1000ff0,0x1001000,0x1002000
    results = []
    for cmd in parsed['commands']:
        source = raw[cmd['file_offset']:cmd['file_offset']+8]
        uc.mem_write(buf,source + b'\xa5'*16)
        uc.mem_write(stack,struct.pack('<I',stop))
        uc.reg_write(UC_X86_REG_ESP,stack)
        uc.reg_write(UC_X86_REG_ECX,buf)
        uc.reg_write(UC_X86_REG_EAX,0)
        # Isolated interpreter: no Windows loader or native game execution.
        uc.emu_start(0x408b6a,stop,count=10000)
        assert uc.reg_read(UC_X86_REG_EIP)==stop, 'Did not return within instruction budget'
        assert uc.reg_read(UC_X86_REG_ESP)==stack+4, 'Unexpected stack delta'
        converted = bytes(uc.mem_read(buf,8))
        expected = struct.pack('<hbbI',cmd['size'],cmd['verb'],cmd['player'],cmd['tick'])
        assert converted == expected, (converted.hex(),expected.hex())
        assert bytes(uc.mem_read(buf+8,16))==b'\xa5'*16, 'Wrote beyond header'
        results.append(dict(file_offset=cmd['file_offset'], input_hex=source.hex(),
                            original_x86_output_hex=converted.hex(),matches=True))
    result = dict(result='pass',binary_sha256=hashlib.sha256(binary).hexdigest(),
                  film_sha256=hashlib.sha256(raw).hexdigest(), original_routine_va='0x00408b6a',
                  checked_headers=len(results), comparisons=results,
                  limitations='Only command header conversion. Command effects, gameplay state and replay determinism not evaluated.')
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(f"PASS: {len(results)} real Film headers match the original x86 conversion routine.")


if __name__ == '__main__':
    main()
