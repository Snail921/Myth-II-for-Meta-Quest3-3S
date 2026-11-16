"""Preserve and inventory user recordings in timestamp order (read-only source)."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import struct
from inspect_film import parse

ROOT = Path(__file__).resolve().parents[1]
GENERAL = ['STOP', 'SCATTER', 'RETREAT', 'SPECIAL_ABILITY', 'GUARD', 'TAUNT', 'CELEBRATE']


def decode(command):
    p = bytes.fromhex(command['payload_hex'])
    def units(offset):
        n, = struct.unpack_from('>h', p, offset)
        if n < 0 or offset + 2 + n * 2 > len(p):
            raise ValueError('Unsupported unit list')
        return list(struct.unpack_from(f'>{n}h', p, offset + 2)), offset + 2 + n * 2
    v = command['verb']
    if v == 2:
        flags, action = struct.unpack_from('>hh', p)
        ids, end = units(4)
        result = dict(flags=flags, action=GENERAL[action] if 0 <= action < len(GENERAL) else action, unit_ids=ids)
    elif v == 4:
        flags, = struct.unpack_from('>H', p)
        ids, pos = units(2)
        targets, end = units(pos)
        result = dict(flags=flags, unit_ids=ids, target_ids=targets)
    elif v == 5:
        flags, x, y, z = struct.unpack_from('>H2xiii', p)
        ids, end = units(16)
        result = dict(flags=flags, raw_position=[x, y, z], unit_ids=ids)
    elif v == 6:
        flags, index, identifier = struct.unpack_from('>H2xHH', p)
        ids, end = units(8)
        result = dict(flags=flags, object_index=index, object_id=identifier, unit_ids=ids)
    else:
        return command.get('movement_candidate')
    result['trailing_hex'] = p[end:].hex()
    return result


def main():
    source = ROOT / 'research/runtime/myth-1.8.1/recordings'
    dest = ROOT / 'research/fixtures/films/combat-2026-11-08'
    output = ROOT / 'research/reports/combat-films'
    dest.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    inventory, seen = [], {}
    for path in sorted(source.iterdir(), key=lambda p: (p.stat().st_mtime_ns, p.name)):
        if not path.is_file():
            continue
        raw = path.read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        parsed = parse(raw)
        saved = dest / (path.name + '.film')
        if saved.exists() and saved.read_bytes() != raw:
            raise ValueError(f'Refusing to replace different fixture: {saved}')
        saved.write_bytes(raw)
        for command in parsed['commands']:
            try:
                candidate = decode(command)
                if candidate is not None:
                    command['semantic_candidate'] = candidate
            except (ValueError, struct.error) as error:
                command['semantic_error'] = str(error)
        row = dict(name=path.name, sha256=digest,
                   source_mtime_utc=datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat(),
                   duplicate_of=seen.get(digest), counts=parsed['command_counts'],
                   roundtrip=parsed['command_stream_roundtrip_equal'], terminal=parsed['terminal_block_seen'])
        seen.setdefault(digest, path.name)
        parsed['source'] = row
        (output / (path.name + '.json')).write_text(json.dumps(parsed, indent=2), encoding='utf-8')
        inventory.append(row)
        print(path.name, json.dumps(row['counts']), 'roundtrip=', row['roundtrip'])
        for c in parsed['commands']:
            if c['verb'] in (2, 4, 5, 6, 8):
                print(' ', c['tick'], c['name_candidate'], c.get('semantic_candidate', c['payload_hex']))
    (output / 'inventory.json').write_text(json.dumps(inventory, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
