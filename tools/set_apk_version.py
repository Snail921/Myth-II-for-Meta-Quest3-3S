"""Write APK binary-manifest version metadata to a separate file for repackaging."""
import argparse
import struct
import zipfile
from pathlib import Path

def version_manifest(data, code, name):
    data = bytearray(data)
    chunks = []
    pos = 8
    strings = []
    while pos < len(data):
        kind, header, size = struct.unpack_from('<HHI', data, pos)
        chunk = bytearray(data[pos:pos + size])
        if kind == 1:
            count, styles, flags, start, style_start = struct.unpack_from('<5I', chunk, 8)
            utf8 = bool(flags & 256)
            def length(offset):
                if utf8:
                    n = chunk[offset]; offset += 1
                    if n & 128: n = ((n & 127) << 8) | chunk[offset]; offset += 1
                else:
                    n = struct.unpack_from('<H', chunk, offset)[0]; offset += 2
                    if n & 32768: n = ((n & 32767) << 16) | struct.unpack_from('<H', chunk, offset)[0]; offset += 2
                return n, offset
            for i in range(count):
                off = start + struct.unpack_from('<I', chunk, header + i * 4)[0]
                n, off = length(off)
                if utf8: n, off = length(off)
                strings.append(bytes(chunk[off:off + n * (1 if utf8 else 2)]).decode('utf8' if utf8 else 'utf-16le'))
            assert len(name) < 128
            encoded = (bytes([len(name), len(name.encode())]) + name.encode() + b'\0') if utf8 else struct.pack('<H', len(name)) + name.encode('utf-16le') + b'\0\0'
            end = style_start or size
            string_data = chunk[start:end] + encoded
            string_data += b'\0' * (-len(string_data) % 4)
            offsets = chunk[header:header + count * 4] + struct.pack('<I', end - start) + chunk[header + count * 4:start]
            result = chunk[:header] + offsets + string_data + (chunk[style_start:] if style_start else b'')
            struct.pack_into('<I', result, 4, len(result))
            struct.pack_into('<I', result, 8, count + 1)
            struct.pack_into('<I', result, 16, flags & ~1) # appended pool is not sorted
            struct.pack_into('<II', result, 20, start + 4, start + 4 + len(string_data) if style_start else 0)
            name_index = count
            chunk = result
        elif kind == 0x102:
            element = struct.unpack_from('<I', chunk, 20)[0]
            if strings[element] == 'manifest':
                attr_start, attr_size, count = struct.unpack_from('<3H', chunk, 24)
                changed = set()
                for i in range(count):
                    off = 16 + attr_start + i * attr_size
                    key = strings[struct.unpack_from('<I', chunk, off + 4)[0]]
                    if key == 'versionCode':
                        struct.pack_into('<I', chunk, off + 8, 0xffffffff)
                        chunk[off + 15] = 0x10
                        struct.pack_into('<I', chunk, off + 16, code)
                        changed.add(key)
                    elif key == 'versionName':
                        struct.pack_into('<I', chunk, off + 8, name_index)
                        chunk[off + 15] = 3
                        struct.pack_into('<I', chunk, off + 16, name_index)
                        changed.add(key)
                assert changed == {'versionCode', 'versionName'}
        chunks.append(chunk)
        pos += size
    result = data[:8] + b''.join(chunks)
    struct.pack_into('<I', result, 4, len(result))
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('apk'); p.add_argument('output'); p.add_argument('code', type=int); p.add_argument('name')
    a = p.parse_args()
    with zipfile.ZipFile(a.apk) as z: data = z.read('AndroidManifest.xml')
    Path(a.output).write_bytes(version_manifest(data, a.code, a.name))
