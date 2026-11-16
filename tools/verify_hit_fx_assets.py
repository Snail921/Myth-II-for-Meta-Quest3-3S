"""Validate hit FX coverage and preserve all previously packaged FX pixels/quads."""
import struct,zipfile,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from prepare_vr_assets import decode,resolved_instance

def unpack(b):
 assert b[:8]==b'MYTHFX02'
 nt,nq=struct.unpack_from('<II',b,8);p=16;tex=[];quads={}
 for _ in range(nt):
  w,h=struct.unpack_from('<II',b,p);p+=8
  assert 0<w<=4096 and 0<h<=4096
  n=w*h*5;tex.append((w,h,b[p:p+n]));assert len(tex[-1][2])==n;p+=n
 for _ in range(nq):
  q=struct.unpack_from('<8I5f',b,p);p+=52
  assert q[7]<nt and q[3]<q[4]
  k=q[:4];assert k not in quads
  quads[k]=(q[:7]+q[8:],tex[q[7]])
 assert p==len(b)
 return quads

new=unpack((ROOT/'build/vr/assets/particles.pack').read_bytes())
old=unpack(zipfile.ZipFile(ROOT/'build/myth-standalone.apk').read('assets/particles.pack'))
for k,v in old.items():assert new[k]==v,('existing FX changed',k)
raw=None
for a in json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf-8')):
 for t in a.get('tags',[]):
  if t['type']=='.256' and t['id_hex']=='67757473':
   with (ROOT/a['path']).open('rb') as f:f.seek(t['offset']);raw=f.read(t['size'])
assert raw is not None
ims,instances,seq=decode(raw);indexed=decode(raw,palette_indices=True)[0];checked=0
for si,(flags,scale,refs) in enumerate(seq):
 for fi,row in enumerate(refs):
  for vi in range(len(row)):
   fl,rx,ry,bi=resolved_instance(ims,instances,flags,row,vi)
   if bi<0:continue
   meta,texture=new[(0x240a1862,si,fi,vi)]
   w,h,pixels=ims[bi]
   assert texture==(w,h,bytes(pixels)+bytes(indexed[bi][2][0::4]))
   assert meta[4:7]==(len(row),flags,fl)
   assert meta[7:]==(float(rx),float(ry),scale,float(w),float(h))
   checked+=1
assert checked==138 and len(seq)==8
added=set(new)-set(old);assert all(k[0]==0x240a1862 for k in added)
result={'result':'pass','unchanged_quads':len(old),'added_quads':len(added),'guts_sequences':len(seq),'guts_direction_frames_verified':checked,'runtime_missing_signature':'240a1862','manual_visual_and_fps':'pending'}
(ROOT/'research/reports/hit-fx-assets-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))
