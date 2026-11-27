"""Validate a converted external level bundle before publishing it to Quest."""
import argparse, hashlib, json, re, struct, math
from pathlib import Path
from export_level_bundle import FILES, OPTIONAL

def verify(bundle):
    base = Path(bundle).resolve()
    rows = (base / 'catalog.tsv').read_text(encoding='utf8').splitlines()
    seen = set()
    result = []
    for line in rows:
        row = line.split('\t')
        if len(row) != 10 or not re.fullmatch('[0-9a-f]{8}', row[0]) or row[0] in seen:
            raise ValueError('Invalid or duplicate catalog row')
        if not re.fullmatch('[0-9a-f]{64}', row[9]) or not row[8] or len(row[8].encode('utf8')) >= 64 or '\0' in row[8]:
            raise ValueError('Invalid level name/identity')
        if not 0 <= int(row[1]) <= 0xffffffff:
            raise ValueError('Invalid landscape ID')
        if any(not 32 <= int(v) <= 512 or int(v) % 32 for v in row[2:4]):
            raise ValueError('Invalid map dimensions')
        if any(not math.isfinite(float(v)) for v in row[4:8]):
            raise ValueError('Invalid map coordinates')
        seen.add(row[0])
        folder = (base / 'levels' / row[0]).resolve()
        if not folder.is_relative_to(base):
            raise ValueError('Level path escapes bundle')
        manifest = json.loads((folder / 'manifest.json').read_text(encoding='utf8'))
        if manifest.get('version') != 1 or manifest.get('catalog') != line or not set(FILES).issubset(manifest['files']) or not set(manifest['files']).issubset(FILES+OPTIONAL):
            raise ValueError('Manifest/catalog/file set mismatch: ' + row[0])
        if 'source_files_sha256' in manifest:
            source = (base / 'source-files.json').read_bytes()
            if len(source) > 65536 or hashlib.sha256(source).hexdigest() != manifest['source_files_sha256']:
                raise ValueError('Source requirements checksum mismatch')
            requirements = json.loads(source)
            if requirements.get('version') != 1 or not requirements.get('files'):
                raise ValueError('Invalid source requirements')
        total = 0
        for name in manifest['files']:
            p = (folder / name).resolve()
            if not p.is_relative_to(folder):
                raise ValueError('Asset path escapes level')
            spec = manifest['files'][name]
            size = p.stat().st_size
            if size != spec['size'] or not 0 <= size <= 512 * 1024**2:
                raise ValueError('Invalid size: ' + str(p))
            with p.open('rb') as f:
                digest = hashlib.file_digest(f, 'sha256').hexdigest()
            if digest != spec['sha256']:
                raise ValueError('Checksum mismatch: ' + str(p))
            total += size
        if total > 1024**3:
            raise ValueError('Level exceeds runtime size limit')
        if 'terrain-base.pack' in manifest['files']:
            color=(folder/'terrain-base.pack').read_bytes()
            width,height=int(row[2])*8,int(row[3])*8
            if len(color)!=1040+width*height*4 or color[:8]!=b'MYTHBC01' or struct.unpack_from('<II',color,8)!=(width,height):
                raise ValueError('Invalid unlit terrain color data')
        b = (folder / 'collections.bin').read_bytes()
        if b[:8] != b'MVRCL002' or len(b) < 12:
            raise ValueError('Plugin bundle requires tagged collection identities')
        n = struct.unpack_from('<I', b, 8)[0]
        if not 1 <= n <= 255 or len(b) != 12 + n * 32:
            raise ValueError('Collection manifest size')
        slots, tags = set(), set()
        for i in range(n):
            slot, *_, tag = struct.unpack_from('<8I', b, 12 + i * 32)
            if slot >= 255 or slot in slots or tag == 0xffffffff or tag in tags:
                raise ValueError('Ambiguous collection identity')
            slots.add(slot); tags.add(tag)
        result.append(dict(key=row[0], name=row[8], collections=n, bytes=total))
    if not result:
        raise ValueError('Empty bundle')
    return dict(levels=result, bytes=sum(r['bytes'] for r in result))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('bundle')
    args = parser.parse_args()
    print(json.dumps(verify(args.bundle), indent=2))
