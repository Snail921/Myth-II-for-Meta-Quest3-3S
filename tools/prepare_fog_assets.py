"""Preserve original map fog color and density, without per-level overrides."""
import struct
from pathlib import Path
from level_assets import inventory,raw_tag
root=Path(__file__).resolve().parents[1]
tags=inventory();rows=[]
for kind,key in sorted(tags):
 if kind!='mesh':continue
 b=raw_tag(tags,kind,key)
 rgb=struct.unpack_from('>3H',b,496);density=struct.unpack_from('>f',b,504)[0]
 rows.append(key.encode('latin1')+struct.pack('<4f',*(v/65535 for v in rgb),density))
(root/'build/vr/assets/fog.pack').write_bytes(struct.pack('<I',len(rows))+b''.join(rows))
print('Fog maps:',len(rows))
