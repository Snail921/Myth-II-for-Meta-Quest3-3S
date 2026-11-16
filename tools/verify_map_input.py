"""Run the compiled input dispatcher with both map dimensions and invalid meshes."""
import struct
import verify_group_selection as t
from level_assets import inventory,raw_tag,parse_mesh,schemas
from campaign_levels import campaign
tags=inventory();sizes=set()
for mid,_ in campaign(tags):
 m=parse_mesh(raw_tag(tags,'mesh',mid),schemas());sizes.add((m['width']//32,m['height']//32))
for w,h in sorted(sizes|{(1,1),(16,16)}):
    t.c.mem_write(t.mesh+8,struct.pack('<HH',w,h))
    assert t.begin(1)==1 and t.chunk([10002])==1 and t.commit(1)==1
    assert t.selected()==[2]
for w,h in [(0,7),(5,0),(17,7),(5,17)]:
    t.c.mem_write(t.mesh+8,struct.pack('<HH',w,h))
    assert t.begin(1)==11
print('PASS compiled selection accepts every official campaign dimension; rejects invalid maps')
