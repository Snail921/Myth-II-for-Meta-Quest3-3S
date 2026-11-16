"""Official single-player entrypoints and immutable mesh-header identity."""
import json, struct, hashlib
from level_assets import ROOT, raw_tag, parse_mesh, schemas

# Numeric/tag fields retained in the original native mesh header. No pointers,
# runtime indices, mutable flags, weather/lighting values or terrain state.
IDENTITY_OFFSETS=(0,4,8,16,36,44,52,60,68,72,80,128,136,140,144,148,152,156,160,176,252,256,284,292)
def identity(raw):
    values=[]
    for offset in IDENTITY_OFFSETS:
        value=struct.unpack_from('>I',raw,offset)[0]
        if offset==8:
            w,h=struct.unpack_from('>HH',raw,offset);value=w|(h<<16)
        values.append(value)
    return hashlib.sha256(struct.pack('<'+'I'*len(values),*values)).hexdigest()

def campaign(tags):
    names={}
    for archive in json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf8')):
        if archive['archive_type']!='FOUNDATION' or not archive['entrypoint_count']:continue
        with (ROOT/archive['path']).open('rb') as f:
            head=f.read(128)
            if head[124:128]!=b'dng2':raise ValueError('entrypoint archive signature')
            rows=f.read(archive['entrypoint_count']*112)
        for identifier,sort_name,name in struct.iter_unpack('>16s32s64s',rows):
            decode=lambda b:b.split(b'\0')[0].decode('mac_roman')
            names[decode(identifier)]=(decode(sort_name),decode(name))
    result=[];seen=set()
    for mapid,(order,name) in sorted(names.items(),key=lambda x:x[1][0]):
        if ('mesh',mapid) not in tags:continue
        raw=raw_tag(tags,'mesh',mapid);mesh=parse_mesh(raw,schemas())
        if not mesh['header']['flags']&65536:continue
        fingerprint=identity(raw)
        if fingerprint in seen:raise ValueError('ambiguous campaign identity')
        seen.add(fingerprint);result.append((mapid,name))
    if not result:raise ValueError('empty campaign')
    return result
