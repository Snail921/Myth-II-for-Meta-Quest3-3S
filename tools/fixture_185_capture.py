"""Capture read-only PC pages for an ARM64 producer/parser contract test."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools');from probe_engine_state import Memory
m=Memory(int(sys.argv[1]));pages={}
def take(a,n):
 for p in range(a&~4095,(a+n+4095)&~4095,4096):
  if p not in pages:pages[p]=m.read(p,4096)
def u(a):take(a,4);return struct.unpack_from('<I',pages[a&~4095],a&4095)[0]
try:
 snap=m.snapshot();tick=snap['tick']
 for a,n in [(0x400000,4096),(0x47b31c,8),(0x531618,4),(0x6bee40,128),(0x6a7b2c,4),(0x532158,64),(0x532e3c,64),(0x67ddb0,256),(0x5332dc,8),(0x67dcf8,4),(0x67e1e8,2560),(0x5319a4,64)]:take(a,n)
 ap=snap['actors'];op=snap['objects'];take(ap['data'],ap['count']*220);take(op['data'],op['count']*80)
 actors=m.read(ap['data'],ap['count']*220);objects=m.read(op['data'],op['count']*80)
 defs=u(0x532118)
 for i in range(ap['count']):
  identity,d=struct.unpack_from('<Hh',actors,i*220)
  if identity and 0<=d<1024:take(defs+d*1024,1024)
 models=u(0x5319a4+48);cnt=struct.unpack('<H',m.read(0x5319a4+42,2))[0];md=u(0x531960)
 if cnt and cnt<1024:
  take(models,cnt*132);raw=m.read(models,cnt*132)
  for i in range(cnt):
   if struct.unpack_from('<H',raw,i*132)[0]:take(md+struct.unpack_from('<H',raw,i*132+4)[0]*32,32)
 col=u(0x533e20);descriptor=u(0x68f650);take(descriptor,20)
 for i in range(op['count']):
  o=objects[i*80:(i+1)*80]
  if not struct.unpack_from('<H',o)[0] or not o[44] or o[42]>=255:continue
  a=col+o[42]*320;take(a,320);head=m.read(a,320);pc=struct.unpack_from('<I',head,24)[0]
  if o[44]>=pc or pc>128:continue
  pt=struct.unpack_from('<I',head,16)[0];v=u(pt+o[44]*4);take(v+156,1024)

 for a in [0x68f6b8,0x540a34,0x638c08]:take(a,4)
 take(snap['mesh_pointer'],1024)
 registry=m.read(0x52cfd0,88);take(0x52cfd0,88);st=u(0x52d020);num=struct.unpack_from('<H',registry,68)[0];ids=struct.unpack_from('<I',registry,84)[0]
 take(ids,num*4)
 for i in range(num):
  take(st+i*16,16);desc=m.read(st+i*16,16);n=struct.unpack_from('<H',desc)[0];ptr=struct.unpack_from('<I',desc,12)[0]
  if 0<n<4096 and ptr>=65536:
   take(ptr,n*4)
   for j in range(n):
    a=u(ptr+j*4)
    if a>=65536:
     try:take(a,1024)
     except Exception:pass
 appbase=u(0x52ebf8)
 for i in range(ap['count']):
  if struct.unpack_from('<H',actors,i*220)[0]:
   app=struct.unpack_from('<H',actors,i*220+0x94)[0]
   if app<1024:take(appbase+app*384,384)
 # Merge cached FX/terrain source pages. This is explicitly a wire-format fixture,
 # not an atomic gameplay snapshot.
 extra=Path('build/integration-185/full-memory.fixture').read_bytes()
 for i in range(struct.unpack_from('<I',extra)[0]):
  a=struct.unpack_from('<I',extra,4+i*4100)[0]
  if a not in pages:pages[a]=extra[8+i*4100:4104+i*4100]
 end_tick=m.u32(0x6bee64)
 out=Path('build/integration-185/memory.fixture')
 with out.open('wb') as f:
  f.write(struct.pack('<I',len(pages)))
  for a,b in sorted(pages.items()):f.write(struct.pack('<I',a)+b)
 Path('research/reports/version-185-fixture.json').write_text(json.dumps({'tick':tick,'pages':len(pages),'actors':ap['count'],'objects':op['count'],'read_only':True,'end_tick':end_tick,'atomic':False,'purpose':'wire-format test, not gameplay consistency'},indent=2))
 print('Fixture:',len(pages),'pages, tick',tick)
finally:m.close()
