"""Validate source geometry coordinate order against original x86 model submission.
Uses real windmill faces in all poses, plus boundary/random UVs and both sides.
The v1 model packs preserve the source pair; GL expects the exchanged pair.
"""
from verify_combat_builders import machine,run,struct,UC_X86_REG_EAX,ROOT
from level_assets import inventory,raw_tag
from prepare_vr_assets import u,s
import random,json
c=machine();p=0x100c000;g=0x100c100;mat=0x100c200;out=0x100d000
c.mem_write(p,bytes(16));c.mem_write(mat,bytes(64))
for j in range(3):c.mem_write(0x6694b0+4*j,struct.pack('<I',0x100e000+24*j))
def check(pairs,back=False):
 c.mem_write(0x67563c,struct.pack('<I',out));c.mem_write(0x675634,struct.pack('<I',out+4096))
 b=bytearray(64);struct.pack_into('<H',b,0,0x23 if back else 1)
 for j,(a,v) in enumerate(pairs):struct.pack_into('<4H',b,28+8*j,a,v,j,0)
 c.mem_write(g,bytes(b));run(c,0x4ba5cb,[p,0,g,p,0,0,0],{UC_X86_REG_EAX:mat})
 actual=[struct.unpack('<2H',c.mem_read(out+28+j*8,4)) for j in range(3)]
 expected=[(v,a) for a,v in pairs]
 assert actual==(expected[::-1] if back else expected),(pairs,actual)
 return 1
r=random.Random(181185);count=0
for i in range(1000):
 pairs=[(r.randrange(65536),r.randrange(65536)) for j in range(3)]
 count+=check(pairs,bool(i&1))
for back in (False,True):count+=check([(0,65535),(65535,0),(32768,1)],back)
tags=inventory();real=0
for name in ['01bg','01bh','01bi','01bj','01bk','01bl','01bm']:
 b=raw_tag(tags,'geom',name);so=u(b,76)+u(b,52)
 for i in range(s(b,12)):
  pairs=[struct.unpack_from('>2H',b,so+i*64+28+j*8) for j in range(3)]
  for back in (False,True):real+=check(pairs,back)
report={'original_submission':'1.8.1 0x004ba5cb (executed in Unicorn)','original_gl_consumer':'1.8.5 0x0044c110 reads output +0x1c as u, +0x1e as v','random_and_boundary_cases':count,'windmill_face_cases':real,'poses':7,'source_order':['v','u'],'gpu_order':['u','v'],'scope':'MYTHMD01 geom triangles excluding generated connectors; all MYTHAM01 triangles','device_visual_confirmation':'pending'}
(ROOT/'research/reports/model-texture-coordinates.json').write_text(json.dumps(report,indent=2))
print('PASS',count,'random/boundary cases and',real,'windmill face cases')
