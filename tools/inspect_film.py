"""Read-only Myth II Film block/command inventory; no network or replay execution.

Format reference: jwheare/mythextract reco_tag.py, pinned commit in research report.
The binary's update_replay path independently confirms 8-byte block/command headers.
Only framing is decoded; command payload semantics remain unverified.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

NAMES = ['NULL','SYNC','GENERAL','MOVEMENT','TARGET','ATTACK_LOCATION','PICK_UP',
         'RENAME','ROTATION','DROP_PLAYER','CHAT','DETACH','UNIT_ADJUSTMENT','SKETCH',
         'ALLY','PRESET_SELECTION','SYNC_FORMATION','INVENTORY','INVENTORY_DROP',
         'INVENTORY_PREV','INVENTORY_NEXT','FORCED_ENDGAME']


def movement_payload(payload):
    """Decode the documented legacy MOVEMENT layout; do not resolve unit identity."""
    if len(payload) < 12:
        raise ValueError('Truncated movement payload')
    flags, formation, facing = struct.unpack_from('>hHH', payload)
    count, = struct.unpack_from('>h', payload, 8)
    cursor = 10 + count * 2
    if count < 0 or cursor + 2 > len(payload):
        raise ValueError('Invalid movement unit count')
    units = list(struct.unpack_from(f'>{count}h', payload, 10))
    waypoint_count, = struct.unpack_from('>h', payload, cursor)
    cursor += 2
    if waypoint_count < 0 or cursor + waypoint_count * 8 > len(payload):
        raise ValueError('Invalid movement waypoint count')
    points = []
    for _ in range(waypoint_count):
        x, y = struct.unpack_from('>ii', payload, cursor)
        points.append(dict(raw_x=x, raw_y=y, x_candidate=x/512, y_candidate=y/512))
        cursor += 8
    return dict(flags=flags, formation_index=formation, final_facing_raw=facing,
                unit_ids=units, waypoints=points, trailing_hex=payload[cursor:].hex(),
                interpretation='Layout from mythextract; unit IDs are not yet mapped to map units. Coordinates are candidate game units, not VR metres.')


def parse(raw):
    if len(raw) < 100 or raw[60:64] != b'mth2' or raw[36:40] != b'reco':
        raise ValueError('Expected a Myth II reco tag with at least 36 bytes of recording header')
    # Standalone recordings in the reference parser have a 64-byte tag header.
    offset, size = struct.unpack_from('>ii', raw, 44)
    declared_size = size
    # The game's Last Recording leaves tag_data_size at zero. The reference
    # reader also reads standalone recording data to EOF rather than using it.
    if size == 0:
        size = len(raw) - offset
    if offset != 64 or size < 36 or offset + size > len(raw):
        raise ValueError('Unsupported or out-of-bounds standalone tag payload')
    data = raw[offset:offset+size]
    version, kind, start, ending, snap_off, snap_len, init_off, init_len, place_off, place_len = struct.unpack_from('>hh8i', data)
    if start < 36 or start > len(data):
        raise ValueError('Invalid command stream offset')
    for name, position, length in [('snapshot',snap_off,snap_len),('initializers',init_off,init_len),('placement',place_off,place_len)]:
        if length < 0 or (length and (position < 0 or position+length > len(data))):
            raise ValueError(f'Out-of-bounds {name}')
    blocks, commands = [], []
    rebuilt_blocks = bytearray()
    pos = start
    terminal = False
    while pos < len(data):
        if len(data)-pos < 8:
            raise ValueError(f'Truncated block header at {pos}')
        flags, length, count = struct.unpack_from('>Ihh', data, pos)
        begin, end = pos+8, pos+8+length
        if length < 0 or length > 2048 or count < 0 or end > len(data):
            raise ValueError(f'Invalid block bounds/count at {pos}')
        cursor = begin
        rebuilt_commands = bytearray()
        for _ in range(count):
            if cursor+8 > end:
                raise ValueError('Truncated command header')
            command_size, verb, player, tick = struct.unpack_from('>hbbI', data, cursor)
            if command_size < 8 or cursor+command_size > end:
                raise ValueError('Invalid command size')
            payload = data[cursor+8:cursor+command_size]
            commands.append(dict(file_offset=offset+cursor, size=command_size, verb=verb,
                                 name_candidate=NAMES[verb] if 0 <= verb < len(NAMES) else 'UNKNOWN',
                                 player=player, tick=tick, payload_hex=payload.hex()))
            if verb == 3:
                try:
                    commands[-1]['movement_candidate'] = movement_payload(payload)
                except ValueError as error:
                    commands[-1]['payload_decode_error'] = str(error)
            rebuilt_commands.extend(struct.pack('>hbbI',command_size,verb,player,tick))
            rebuilt_commands.extend(payload)
            cursor += command_size
        if cursor != end:
            raise ValueError('Block payload not exactly consumed by command count')
        blocks.append(dict(file_offset=offset+pos, flags=flags, size=length, command_count=count))
        rebuilt_blocks.extend(struct.pack('>Ihh',flags,length,count))
        rebuilt_blocks.extend(rebuilt_commands)
        pos = end
        if flags & 1:
            terminal = True
            break
    params = None
    if start >= 36 + 556 and len(data) >= 36 + 556:
        p = 36
        params = dict(scenario_tag=data[p+12:p+16].decode('ascii', errors='replace'),
                      difficulty=struct.unpack_from('>h',data,p+16)[0],
                      initial_random_seed=struct.unpack_from('>I',data,p+24)[0],
                      build=struct.unpack_from('>h',data,p+34)[0],
                      gameplay_version=struct.unpack_from('>i',data,p+36)[0],
                      plugin_count=struct.unpack_from('>h',data,p+42)[0])
    return dict(version=version, kind=kind, ending_tick=ending, command_data_offset=start,
                declared_tag_data_size=declared_size, size_from_eof=declared_size == 0,
                game_parameters_candidate=params,
                block_count=len(blocks), command_count=len(commands),
                terminal_block_seen=terminal, trailing_payload_bytes=len(data)-pos,
                command_stream_roundtrip_equal=bytes(rebuilt_blocks)==data[start:pos],
                trailing_file_bytes=len(raw)-(offset+size),
                command_counts=dict(Counter(c['name_candidate'] for c in commands)),
                blocks=blocks, commands=commands,
                limitations='Framing only. Names follow third-party enum. No payload simulation, state checksums or load-order verification.')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('film', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    raw = args.film.read_bytes()
    result = parse(raw)
    result.update(path=str(args.film), sha256=hashlib.sha256(raw).hexdigest())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k not in ('blocks','commands')}, indent=2))


if __name__ == '__main__':
    main()
