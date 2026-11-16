"""Resolve the recorded load order and retain one original mesh for porting tests.

Layout source: pinned MIT-licensed jwheare/mythextract, scripts/mesh_tag.py.
Only literal struct format/name pairs are read; no third-party parser code runs.
This validates asset structure; it does not execute mission actions or a game tick.
"""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / 'research/references/mythextract/scripts/mesh_tag.py'


def schemas():
    text = REFERENCE.read_text(encoding='utf-8')
    result = {}
    for name, expected_size in [('MeshHeaderFmt',1024), ('MeshCellFmt',12),
                                ('MarkerHeadFmt',64), ('MarkerPaletteEntryFmt',32), ('ActionHeadFmt',64)]:
        match = re.search(r'^' + name + r" = \('[^']+', \[\n(.*?)\n\]\)", text, re.M | re.S)
        if not match:
            raise ValueError(f'Missing reference layout {name}')
        fields, offset = [], 0
        for fmt, field, _ in re.findall(r"^\s*\('([^']+)',\s*(?:'([^']+)'|(None))", match[1], re.M):
            size = struct.calcsize('>' + fmt)
            fields.append(dict(format=fmt, name=field or None, offset=offset, size=size))
            offset += size
        if offset != expected_size:
            raise ValueError(f'Changed reference layout: {name}, size {offset}')
        result[name] = dict(size=offset, fields=fields)
    return result


def decode(layout, data, offset=0):
    if offset < 0 or offset + layout['size'] > len(data):
        raise ValueError('Structure outside input')
    result = {}
    for field in layout['fields']:
        if field['name']:
            value, = struct.unpack_from('>' + field['format'], data, offset + field['offset'])
            result[field['name']] = value.decode('latin1') if isinstance(value, bytes) else value
    return result


def parse_mesh(data, layouts):
    header = decode(layouts['MeshHeaderFmt'], data)
    width, height = header['submesh_width'] * 32, header['submesh_height'] * 32
    if not width or not height or header['mesh_size'] != width * height * 12:
        raise ValueError('Invalid mesh dimensions or cell size')
    if header['data_offset'] != 1024 or 1024 + header['data_size'] > len(data):
        raise ValueError('Unsupported mesh body location or length')
    sections = {}
    for name, size_field in [('mesh','mesh_size'), ('marker_palette','marker_palette_size'),
                             ('markers','markers_size'), ('map_actions','map_action_buffer_size'),
                             ('media_coverage_region','media_coverage_region_size'),
                             ('mesh_LOD_data','mesh_LOD_data_size'), ('connectors','connectors_size')]:
        start, size = header[name + '_offset'], header[size_field]
        if start + size > header['data_size']:
            raise ValueError(f'{name} outside declared body')
        sections[name] = dict(offset=1024 + start, size=size)
    occupied = sorted((s['offset'], s['offset']+s['size'], n) for n,s in sections.items() if s['size'])
    for previous, current in zip(occupied, occupied[1:]):
        if previous[1] > current[0]:
            raise ValueError(f'Overlapping mesh sections: {previous[2]}, {current[2]}')

    palette, grouped = [], defaultdict(list)
    for name, count_field, stride in [('marker_palette','marker_palette_entries',32), ('markers','marker_count',64), ('connectors','connector_count',64)]:
        if sections[name]['size'] != header[count_field] * stride:
            raise ValueError(f'{name} count/size mismatch')
    for i in range(header['marker_palette_entries']):
        entry = decode(layouts['MarkerPaletteEntryFmt'], data, sections['marker_palette']['offset'] + i * 32)
        entry['index_in_type'] = len(grouped[entry['type']])
        grouped[entry['type']].append(entry)
        palette.append(entry)
    markers, seen = [], set()
    for i in range(header['marker_count']):
        marker = decode(layouts['MarkerHeadFmt'], data, sections['markers']['offset'] + i * 64)
        choices = grouped[marker['type']]
        if marker['palette_index'] >= len(choices):
            raise ValueError(f'Marker {i} has missing palette entry')
        key = marker['type'], marker['id']
        if key in seen:
            raise ValueError(f'Duplicate marker key {key}')
        seen.add(key)
        entry = choices[marker['palette_index']]
        marker.update(tag=entry['marker_tag'], team_index=entry['team_index'])
        markers.append(marker)

    action_section = sections['map_actions']
    action_bytes = data[action_section['offset']:action_section['offset'] + action_section['size']]
    table_size = header['map_action_count'] * 64
    if table_size > len(action_bytes):
        raise ValueError('Action headers outside action section')
    actions, action_ids = [], set()
    for i in range(header['map_action_count']):
        action = decode(layouts['ActionHeadFmt'], action_bytes, i * 64)
        begin = table_size + action['offset']
        end = begin + action['size']
        if end > len(action_bytes) or action['id'] in action_ids:
            raise ValueError('Invalid action payload or duplicate action ID')
        action_ids.add(action['id'])
        params, cursor = [], begin
        for _ in range(action['num_params']):
            if cursor + 8 > end:
                raise ValueError('Truncated action parameter')
            kind, count, name = struct.unpack_from('>HH4s', action_bytes, cursor)
            if kind > 20:
                raise ValueError(f'Unknown parameter kind {kind}')
            # Original Build 434 table at 0x52cee8, alignment routine 0x49cb27.
            element_sizes = (1,1,2,2,2,4,4,4,4,4,4,4,4,8,16,2,2,2,12,2,2)
            size = (count * element_sizes[kind] + 3) & ~3
            if cursor + 8 + size > end:
                raise ValueError('Parameter data outside action')
            raw = action_bytes[cursor + 8:cursor + 8 + size]
            params.append(dict(type=kind, count=count, name=name.decode('latin1'), raw_hex=raw.hex()))
            cursor += 8 + size
        action.update(parameters=params, unparsed_tail_hex=action_bytes[cursor:end].hex())
        actions.append(action)
    heights = [struct.unpack_from('>h', data, sections['mesh']['offset'] + i * 12)[0] for i in range(width * height)]
    return dict(header=header, width=width, height=height, cell_count=width*height,
                height_raw_min=min(heights), height_raw_max=max(heights), sections=sections,
                trailing_bytes=len(data)-1024-header['data_size'], palette=palette, markers=markers, actions=actions)


def main():
    inventory = json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf-8'))
    by_name = {Path(item['path']).name:item for item in inventory}
    log = (ROOT/'research/reports/runtime-181-startup.txt').read_text(encoding='utf-8')
    found = re.search(r'DEBUG: Adding (\d+) monolithic tags:\n((?:\t[^\n]*\n)+)', log)
    if not found:
        raise ValueError('No observed archive order')
    order = [line.strip() for line in found[2].splitlines()]
    if len(order) != int(found[1]):
        raise ValueError('Archive count mismatch')
    effective, history = {}, defaultdict(list)
    for name in order:
        archive = by_name[name]
        for tag in archive['tags']:
            key = tag['type'], bytes.fromhex(tag['id_hex']).decode('latin1')
            record = dict(archive=archive['path'], archive_sha256=archive['sha256'], **tag)
            history[key].append(record)
            effective[key] = record
    record = effective['mesh','l1  ']
    with (ROOT/record['archive']).open('rb') as source:
        source.seek(record['offset'])
        data = source.read(record['size'])
    if len(data) != record['size']:
        raise ValueError('Truncated archive read')
    layouts = schemas()
    decoded = parse_mesh(data, layouts)
    target = ROOT/'research/fixtures/maps/willow-creek'
    target.mkdir(parents=True, exist_ok=True)
    (target/'mesh.bin').write_bytes(data)
    (target/'reference-layouts.json').write_text(json.dumps(layouts,indent=2),encoding='utf-8')
    (target/'decoded-map.json').write_text(json.dumps(decoded,indent=2,ensure_ascii=True),encoding='utf-8')
    tag_types = {1:'scen',3:'unit',5:'amso',6:'mode',9:'proj',10:'lpgr',11:'anim'}
    dependencies = []
    for item in decoded['palette']:
        kind = tag_types.get(item['type'])
        if kind and item['marker_tag'] != '\xff'*4:
            resolved = effective.get((kind,item['marker_tag']))
            dependencies.append(dict(type=kind,id=item['marker_tag'],resolved=resolved))
    manifest = dict(map_id='l1  ', source=record, source_sha256=hashlib.sha256(data).hexdigest(),
                    observed_archive_order=order, override_rule='last loaded definition, matching pinned reference loader; engine resolution not yet independently verified',
                    reference_sha256=hashlib.sha256(REFERENCE.read_bytes()).hexdigest(),
                    dimensions=[decoded['width'],decoded['height']], marker_count=len(decoded['markers']),
                    marker_types=dict(Counter(m['type'] for m in decoded['markers'])), action_count=len(decoded['actions']),
                    action_types=dict(Counter(m['type'] for m in decoded['actions'])), direct_palette_dependencies=dependencies,
                    missing_palette_dependencies=sum(d['resolved'] is None for d in dependencies),
                    limitations='Asset structure only. No actor instantiation, terrain physics, mission execution, tick advancement, rendering or complete transitive asset resolution.')
    (target/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=True),encoding='utf-8')
    print(json.dumps({k:manifest[k] for k in ['dimensions','marker_count','marker_types','action_count','action_types','missing_palette_dependencies']}))


if __name__ == '__main__':
    main()
