"""Inventory original frame shadow/light metadata without changing VR assets."""
import collections
import json
import struct
from pathlib import Path
from prepare_vr_assets import u, s

ROOT = Path(__file__).resolve().parents[1]

def inspect(raw):
    d = u(raw, 248)
    def bounded(at, size):
        if at < 0 or size < 0 or at + size > len(raw):
            raise ValueError('effect metadata out of bounds')
    count = u(raw, 144)
    bounded(d + u(raw, 148), count * 32)
    frames = []
    for seq in range(u(raw, 128)):
        r = d + u(raw, 132) + seq * 128
        bounded(r, 128)
        p = d + u(raw, r + 64)
        bounded(p, 64)
        views, n = struct.unpack_from('>2h', raw, p + 8)
        if views == 0 and n == 0 and u(raw, r + 68) == 64:
            frames.append(dict(sequence=seq, name=raw[r:r+64].split(b'\0')[0].decode('mac_roman'), maps=[]))
            continue
        if not 0 < views <= 64 or not 0 < n <= 256:
            raise ValueError('sequence dimensions')
        bounded(p + 64, n * (46 + 2 * views))
        refs = [s(raw, p + 64 + f * (46 + 2 * views)) for f in range(n)]
        if any(x < -1 or x >= count for x in refs):
            raise ValueError('frame effect reference')
        frames.append(dict(sequence=seq, name=raw[r:r+64].split(b'\0')[0].decode('mac_roman'),
                           maps=refs))
    used={x for frame in frames for x in frame['maps'] if x>=0}
    maps = []
    for i in range(count):
        p = d + u(raw, 148) + i * 32
        flags, rx, ry, bitmap = struct.unpack_from('>I2ih', raw, p)
        if bitmap >= u(raw, 96):
            if i in used:raise ValueError('effect bitmap index')
            bitmap=-1 # Unreferenced stale metadata is not a renderable shadow.
        maps.append(dict(index=i, flags=flags, kind='light' if flags & 1 else 'shadow',
                         registration_raw=[rx, ry], bitmap=bitmap))
    for frame in frames:frame['kinds']=sorted({maps[x]['kind'] for x in frame['maps'] if x>=0})
    return dict(maps=maps, sequences=frames,
                counts=dict(collections.Counter(m['kind'] for m in maps)))

def main():
    base = ROOT / 'build/scene-bridge'
    scene = json.loads((base/'scene.json').read_text(encoding='utf-8'))
    result = []
    for a in scene['assets']:
        item = inspect((base/a['file']).read_bytes())
        item.update(collection=a['runtime_index'], file=a['file'])
        result.append(item)
    out = ROOT/'research/reports/surface-effects-inventory.json'
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    for item in result:
        print(item['collection'], item['counts'])
        for seq in item['sequences']:
            if 'light' in seq['kinds']:
                print('  light sequence:', seq['sequence'], seq['name'], seq['maps'])

if __name__ == '__main__':
    main()
