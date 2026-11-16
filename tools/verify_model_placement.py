"""Compare all packed model vertices with original Build434 matrix/point transforms.
Only pool allocation and unrelated registration are stubbed; transform instructions run.
"""
import json,struct,math
from verify_combat_builders import *
from prepare_vr_assets import u as be32,s as be16
c=machine();c.mem_write(0x67f2bc,struct.pack('<f',math.tau));c.mem_write(0x529d4c,struct.pack('<I',0x100d000))
def stub(cpu,a,size,user):
 if a not in [0x428832,0x438ade,0x4386b1,0x43891f]:return
 esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
 cpu.reg_write(UC_X86_REG_EAX,1 if a==0x4386b1 else 0);cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,stub)
inv=json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf8'));tags={}
for a in inv:
 if 'disc' in a['path']:
  for t in a.get('tags',[]):tags[t['type'],t['id']]=(a['path'],t)
def get(typ,tag):
 a,t=tags[typ,tag]
 with (ROOT/a).open('rb') as f:f.seek(t['offset']);return f.read(t['size'])
mesh=json.loads((ROOT/'research/fixtures/maps/willow-creek/decoded-map.json').read_text());markers={(m['type'],m['id']):m for m in mesh['markers']}
records=json.loads((ROOT/'research/reports/vr-models.json').read_text());pack=(ROOT/'build/vr/assets/models.pack').read_bytes();nt,nc=struct.unpack_from('<II',pack,8);p=16
for _ in range(nt):w,h=struct.unpack_from('<II',pack,p);p+=8+w*h*4
count=0;results=[]
for rec in records:
 m=markers[11 if rec['animated_first_pose_only'] else 6,rec['marker_id']];g=get('geom',rec['geometry']);center=struct.unpack_from('>3h',g,16);d=be32(g,76);vo=d+be32(g,40);so=d+be32(g,52)
 c.mem_write(0x100e000,struct.pack('<hhiiiHHh',0,0,m['pos_x'],m['pos_y'],m['pos_z'],m['yaw'],49152,-1));run(c,0x437531,[0x100e000],{})
 c.mem_write(0x100e110,struct.pack('<3h',*center));error=0
 for i in range(rec['triangles']):
  values=struct.unpack_from('<I15f',pack,p);p+=64
  for j in range(3):
   index=be16(g,so+i*64+32+j*8);v=struct.unpack_from('>3h',g,vo+index*6);c.mem_write(0x100e100,struct.pack('<3h',*v))
   run(c,0x42cfee,[0x100d008,0x100e120],{UC_X86_REG_EAX:0x100e100,UC_X86_REG_ECX:0x100d060,UC_X86_REG_EDX:0x100e110})
   original=struct.unpack('<3i',c.mem_read(0x100e120,12));actual=values[1+j*5:4+j*5]
   err=max(abs(a*512-b) for a,b in zip(actual,original));error=max(error,err);assert err<.02,(rec,actual,original);count+=1
 results.append(dict(marker=rec['marker_id'],tag=rec['tag'],max_raw_error=error))
assert p==len(pack)
(ROOT/'research/reports/model-placement-verification.json').write_text(json.dumps(dict(vertices=count,models=results,stubbed='allocation and non-transform registration only',result='pass'),indent=2))
print('PASS original model transform:',len(records),'models,',count,'packed vertices')
