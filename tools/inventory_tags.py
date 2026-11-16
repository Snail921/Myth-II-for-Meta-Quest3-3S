"""Inventory original archives using a pinned mythextract reference parser.

Does not extract assets or resolve archive override/load order.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / 'research/references/mythextract'
sys.path.insert(0, str(REFERENCE / 'scripts'))
import myth_headers


def main():
    files = sorted((ROOT / 'research/extracted/disc/tags').glob('*'))
    files += sorted((ROOT / 'research/extracted/update-1.8.1/plugins').glob('*'))
    results = []
    for path in files:
        if not path.is_file():
            continue
        data = path.read_bytes()
        header = myth_headers.parse_mono_header(path.name, data)
        if header.tag_list_start + header.tag_list_size > len(data):
            raise ValueError(f'Tag table out of bounds: {path}')
        tags = myth_headers.get_mono_tags(data, header)
        records = []
        for tag in tags:
            start, size = tag.tag_data_offset, tag.tag_data_size
            if start < 0 or size < 0 or start + size > len(data):
                raise ValueError(f'Tag outside file: {path}: {tag}')
            records.append(dict(type=str(tag.tag_type), id=str(tag.tag_id), name=str(tag.name),
                                type_hex=bytes(tag.tag_type).hex(), id_hex=bytes(tag.tag_id).hex(),
                                offset=start, size=size))
        counts = dict(sorted(Counter(t['type'] for t in records).items()))
        results.append(dict(path=str(path.relative_to(ROOT)), size=len(data),
                            sha256=hashlib.sha256(data).hexdigest(),
                            archive_type=header.type.name, tag_count=header.tag_count,
                            entrypoint_count=header.entry_tag_count, types=counts, tags=records))
        print(f'{path.name}: {len(records)} tags, {header.entry_tag_count} entrypoints; {counts}')
    output = ROOT / 'research/reports/tag-inventory.json'
    output.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
