"""Original meli direction interpreted consistently with existing skybox/ground shadows."""
import struct,math
from pathlib import Path
from level_assets import inventory,raw_tag,parse_mesh,schemas
root=Path(__file__).resolve().parents[1]
tags=inventory();rows=[]
for kind,key in sorted(tags):
 if kind!='mesh':continue
 mesh=parse_mesh(raw_tag(tags,kind,key),schemas());tag=mesh['header']['mesh_lighting_tag']
 if ('meli',tag) not in tags:continue
 i,j,k=struct.unpack_from('>3i',raw_tag(tags,'meli',tag));v=(i/65536.,-k/65536.,-j/65536.);length=math.sqrt(sum(x*x for x in v))
 if length>0 and v[1]>0:rows.append(key.encode('latin1')+struct.pack('<3f',*(x/length for x in v)))
(root/'build/vr/assets/water-light.pack').write_bytes(struct.pack('<I',len(rows))+b''.join(rows));print('Water light maps:',len(rows))
