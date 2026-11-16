"""Validate shipped campaign packs against renderer bounds and the official inventory."""
import struct,json,math
from pathlib import Path
from level_assets import ROOT,inventory,raw_tag
from campaign_levels import campaign,identity

def validate():
 tags=inventory();expected={m.encode('latin1').hex():(m,n) for m,n in campaign(tags)}
 folder=ROOT/'build/vr/assets/levels';rows=[r.split('\t') for r in (folder/'catalog.tsv').read_text(encoding='utf8').splitlines()]
 assert len(rows)==len(expected) and {r[0] for r in rows}==set(expected)
 total=0
 for row in rows:
  assert len(row)==10;key=row[0];mapid,name=expected[key];assert row[8]==name and row[9]==identity(raw_tag(tags,'mesh',mapid))
  w,h=map(int,row[2:4]);assert 32<=w<=512 and 32<=h<=512 and w%32==h%32==0
  path=folder/key
  for filename in ('scene.pack','models.pack','sprite-indices.pack','animated-models.pack','surface-effects.pack','particles.pack','reflection-water.pack','collections.bin','minimap.rgba'):
   assert (path/filename).is_file();total+=(path/filename).stat().st_size
  minimap=(path/'minimap.rgba').read_bytes();mw,mh=struct.unpack_from('<II',minimap);assert 0<mw<=1024 and 0<mh<=1024 and len(minimap)==8+mw*mh*4
  with (path/'scene.pack').open('rb') as f:
   assert f.read(8)==b'MYTHVR01';sw,sh,nt,nq=struct.unpack('<4I',f.read(16));assert (sw,sh)==(w,h)
   f.seek(w*h*2,1)
   for i in range(nt):
    tw,th=struct.unpack('<II',f.read(8));assert 0<tw<=4096 and 0<th<=4096;f.seek(tw*th*4,1)
   axial=0
   for i in range(nq):
    q=struct.unpack('<8I5f',f.read(52));assert q[0]<255 and q[7]<nt and q[3]<q[4] and 0<q[4]<=64;assert all(math.isfinite(x) for x in q[8:]);axial+=bool(q[6]&0x40000000)
   assert f.read(1)==b''
  with (path/'surface-effects.pack').open('rb') as f:assert f.read(8)==b'MYTHSF01' and struct.unpack('<II',f.read(8))==(w*8,h*8)
  with (path/'particles.pack').open('rb') as f:
   assert f.read(8) in (b'MYTHFX02',b'MYTHFX03');nt,nq=struct.unpack('<II',f.read(8));assert nt<=20000 and nq<=100000
   for i in range(nt):
    tw,th=struct.unpack('<II',f.read(8));assert 0<tw<=4096 and 0<th<=4096;f.seek(tw*th*5,1)
   for i in range(nq):
    q=struct.unpack('<8I5f',f.read(52));assert q[1]<=255 and q[2]<=255 and q[3]<q[4]<=64 and q[7]<nt and math.isfinite(q[10]) and q[10]>0
   assert not f.read(1)
  print('PASS',ascii(name),'size',w,h,'axial_frames',axial)
 print('PASS campaign packs:',len(rows),'uncompressed bytes:',total)
if __name__=='__main__':validate()
