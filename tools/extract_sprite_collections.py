"""Extract Myth II .256 and d256 collection payloads from one plugin."""
import argparse
import hashlib
import mmap
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'build/map-readers'))
from myth_headers import parse_mono_header, get_mono_tags


def extract(plugin, output, tag_id=None):
    with plugin.open('rb') as file, mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
        header = parse_mono_header(plugin.name, data)
        if header.game_version != 2:
            raise ValueError('Expected a Myth II archive')
        for tag in get_mono_tags(data, header):
            kind = bytes(tag.tag_type)
            if kind not in (b'.256', b'd256') or (tag_id and str(tag.tag_id) != tag_id):
                continue
            start, size = tag.tag_data_offset, tag.tag_data_size
            if start < 0 or size <= 0 or start + size > len(data):
                raise ValueError('Invalid collection bounds')
            payload = data[start:start+size]
            name = re.sub(r'[^\w .()-]', '_', str(tag.name)).strip(' .') or 'collection'
            digest = hashlib.sha256(payload).hexdigest()[:12]
            folder = output / ('HD Collections' if kind == b'd256' else 'UniqueCollections')
            folder.mkdir(parents=True, exist_ok=True)
            dest = folder / (name + '__' + digest + ('.d256' if kind == b'd256' else '.bin'))
            if dest.exists():
                if dest.read_bytes() != payload:
                    raise ValueError(f'Existing file differs: {dest}')
            else:
                with dest.open('xb') as out:
                    out.write(payload)
            print(dest)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('plugin', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--id', help='Optional four-character tag ID')
    args = parser.parse_args()
    extract(args.plugin, args.output, args.id)
