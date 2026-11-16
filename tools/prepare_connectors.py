"""Export static connector chains as two-sided, alpha-textured world geometry.

Only the declared chain length is valid: unused ID slots contain editor garbage.
The original connector texture is repeated along each span, not stretched once.
Exports per-span keys; protocol v17 gates drawing with native active edges.
"""
import math
import struct
from prepare_vr_assets import decode


def append_connectors(mesh, get, textures, triangles, map_tag=None):
    h = mesh['header']
    count = h.get('connector_count', 0)
    if not count:
        return {'chains': 0, 'spans': 0}
    raw = get('mesh', map_tag or h['landscape_collection_tag'])
    # Callers supply the actual mesh tag when it differs from the landscape.
    offset = 1024 + h['connectors_offset']
    if offset + count * 64 > len(raw):
        raise ValueError('Connector bounds')
    conn = get('conn', h['connector_tag'])
    col = conn[4:8].decode('mac_roman')
    seq_index, fraction, spacing = struct.unpack_from('>hhh', conn, 8)
    ims, instances, sequences = decode(get('.256', col))
    flags, scale, refs = sequences[seq_index]
    _, rx, ry, bitmap = instances[refs[0][0]]
    w, height, pixels = ims[bitmap]
    texture = len(textures)
    textures.append((w, height, pixels))
    period = w * scale * 10 / 512
    rise = height * scale * 10 / 512
    markers = {m['id']: m for m in mesh['markers'] if m['type'] == 1}
    tops = {}
    def point(identity):
        m = markers[identity]
        tag = m['tag'].encode('latin1').decode('mac_roman')
        if tag not in tops:
            scenery = get('scen', tag)
            si = struct.unpack_from('>h', scenery, 40)[0]
            reference = scenery[4:8].decode('mac_roman')
            try:
                collection = get('core', reference)[:4].decode('mac_roman')
            except KeyError:
                collection = reference
            a, inst, seq = decode(get('.256', collection))
            _, _, py, _ = inst[seq[si][2][0][0]]
            tops[tag] = py * seq[si][1] * 10 / 512
        return (m['pos_x']/512, m['pos_y']/512,
                m['pos_z']/512 + tops[tag] * fraction/256)
    spans = 0
    ranges = []
    for i in range(count):
        ids = struct.unpack_from('>32H', raw, offset+i*64)
        n = ids[31]
        if n > 31:
            raise ValueError('Connector chain length')
        for first, second in zip(ids[:n], ids[1:n]):
            if first not in markers or second not in markers:
                raise ValueError('Missing connector endpoint')
            a, b = point(first), point(second)
            ranges.append([len(triangles),4,markers[first]['pos_x'],markers[first]['pos_y'],markers[second]['pos_x'],markers[second]['pos_y'],round((height-ry)*scale*10),round(height*scale*10)])
            length = math.dist(a[:2], b[:2])
            if length == 0:
                continue
            uv = length/period
            vertices = [(*a,0.,0.), (*b,uv,0.),
                        (b[0],b[1],b[2]-rise,uv,1.),
                        (a[0],a[1],a[2]-rise,0.,1.)]
            for indices in [(0,1,2),(0,2,3),(2,1,0),(3,2,0)]:
                triangles.append((texture,[v for j in indices for v in vertices[j]]))
            spans += 1
    return {'chains':count,'spans':spans,'texture':col,
            'endpoint_height':'visual sprite height approximation',
            'dynamic_endpoint_destruction':True,'ranges':ranges}
