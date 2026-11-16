"""Read-only PE inventory for local Myth binaries; uses Python's standard library."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def inspect(path):
    data = path.read_bytes()

    def unpack(fmt, offset):
        return struct.unpack_from('<' + fmt, data, offset)

    def string(offset):
        end = data.find(b'\0', offset)
        if end < 0:
            raise ValueError('Unterminated PE string')
        return data[offset:end].decode('ascii', errors='replace')

    if data[:2] != b'MZ':
        raise ValueError('Not an MZ executable')
    pe, = unpack('I', 0x3c)
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not a PE executable')
    machine, count, timestamp, symbol_table, symbols, opt_size, flags = unpack('HHIIIHH', pe + 4)
    opt = pe + 24
    magic, = unpack('H', opt)
    if magic not in (0x10b, 0x20b):
        raise ValueError('Unsupported optional header')
    is64 = magic == 0x20b
    image_base, = unpack('Q' if is64 else 'I', opt + (24 if is64 else 28))
    entry, = unpack('I', opt + 16)
    headers_size, = unpack('I', opt + 60)
    sections = []
    for i in range(count):
        start = opt + opt_size + 40 * i
        name = data[start:start + 8].split(b'\0')[0].decode('ascii', errors='replace')
        vs, va, rs, rp = unpack('IIII', start + 8)
        sections.append(dict(name=name, virtual_size=vs, rva=va, raw_size=rs, raw_offset=rp))

    def offset(rva):
        if rva < headers_size and rva < len(data):
            return rva
        for s in sections:
            delta = rva - s['rva']
            if 0 <= delta < s['raw_size'] and s['raw_offset'] + delta < len(data):
                return s['raw_offset'] + delta
        raise ValueError(f'Unmapped file RVA: {rva:#x}')

    dir_count, = unpack('I', opt + (108 if is64 else 92))
    directories = opt + (112 if is64 else 96)
    imports = []
    if dir_count > 1:
        imp_rva, imp_size = unpack('II', directories + 8)
        if imp_rva:
            p = offset(imp_rva)
            while True:
                lookup, stamp, forward, name_rva, iat = unpack('IIIII', p)
                if not any((lookup, stamp, forward, name_rva, iat)):
                    break
                funcs = []
                q = offset(lookup or iat)
                width = 8 if is64 else 4
                ordinal_mask = 1 << (width * 8 - 1)
                while True:
                    value, = unpack('Q' if is64 else 'I', q)
                    if value == 0:
                        break
                    funcs.append({'ordinal': value & 0xffff} if value & ordinal_mask else {'name': string(offset(value) + 2)})
                    q += width
                imports.append({'dll': string(offset(name_rva)), 'functions': funcs})
                p += 20
    anchors = []
    pattern = re.compile(r'(?i)(\.cpp\b|\.c\b|\.h\b|\.pdb\b|assert|random|tick|film|replay|mesh|raster|render|direct3d|opengl|collection|projectile|pathfind|tag file|checksum|out.of.sync|simulation)')
    for match in re.finditer(rb'[\x20-\x7e]{6,}', data):
        value = match.group().decode('ascii')
        if pattern.search(value):
            rva = None
            for s in sections:
                if s['raw_offset'] <= match.start() < s['raw_offset'] + s['raw_size']:
                    rva = s['rva'] + match.start() - s['raw_offset']
                    break
            anchors.append({'file_offset': match.start(), 'rva': rva, 'va': image_base + rva if rva is not None else None, 'text': value})
    return dict(path=str(path), size=len(data), sha256=hashlib.sha256(data).hexdigest(),
                machine=hex(machine), pe_bits=64 if is64 else 32, timestamp_raw=timestamp,
                image_base=image_base, entry_rva=entry, coff_symbol_count=symbols,
                sections=sections, imports=imports, string_anchors=anchors,
                limitations='Static normal imports and ASCII strings only. Delay imports, dynamic DLL loads, runtime paths and function boundaries are not resolved.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+', type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    results = [inspect(p) for p in args.files]
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    for result in results:
        print(f"{result['path']}: {result['pe_bits']}-bit, {len(result['string_anchors'])} anchors")
        print('  Imports: ' + ', '.join(i['dll'] for i in result['imports']))


if __name__ == '__main__':
    main()
