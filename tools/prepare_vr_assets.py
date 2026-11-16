from build_progress import tracked
"""Bounded decoder for original Myth .256 assets; format reference: mythextract (MIT)."""
import struct,json
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions
def u(b,o):return struct.unpack_from('>I',b,o)[0]
def s(b,o):return struct.unpack_from('>h',b,o)[0]
def decode(b, palette_indices=False):
 d=u(b,248);c=d+u(b,68);n=u(b,c)
 palette=[tuple(b[c+32+i*8+j] for j in (0,2,4)) for i in range(n)]
 images=[]
 for i in range(u(b,96)):
  r=d+u(b,100)+i*128;p=d+u(b,r+64);end=p+u(b,r+68)
  w,h,bpr,flags=struct.unpack_from('>HHhH',b,p)
  if not 0<w<=4096 or not 0<h<=4096:raise ValueError('bitmap dimensions')
  enc=struct.unpack_from('>H',b,p+16)[0] if flags&16384 else 0
  if enc:raise ValueError(f'Unsupported extended bitmap {enc}')
  p+=52+(0 if flags&32 else 4*(h-1));pixels=bytearray(w*h*4)
  def color(index,alpha):
   if u(b,4)&256 and i&1:return (index,index,index,255)
   if index>=len(palette):raise ValueError('palette index')
   if palette_indices:return (index,index,index,alpha)
   return (*palette[index],alpha)
  for y in range(h):
   if flags&4:
    count,np=struct.unpack_from('>HH',b,p);p+=4
    if count>w or np>w:raise ValueError('spans')
    spans=[struct.unpack_from('>HH',b,p+j*4) for j in range(count)];p+=count*4
    prev=0;total=0
    for x0,x1 in spans:
     if not prev<=x0<=x1<=w:raise ValueError('span order')
     for x in range(x0,x1):
      alpha=255
      if flags&16:alpha=(15-(b[p]&15))*17;p+=1
      ix=b[p];p+=1
      rgba=color(ix,alpha if ix else 0);pixels[(y*w+x)*4:(y*w+x)*4+4]=bytes(rgba)
     prev=x1;total+=x1-x0
    if total!=np:raise ValueError('span pixel count')
    if flags&64:p+=(p-(d+u(b,r+64)+52+(0 if flags&32 else 4*(h-1))))&1
   else:
    for x in range(w):
     alpha=255
     if flags&16:alpha=(15-(b[p]&15))*17;p+=1
     ix=b[p];p+=1
     if ix==0 and not u(b,4)&256:alpha=0
     pixels[(y*w+x)*4:(y*w+x)*4+4]=bytes(color(ix,alpha))
   if p>end:raise ValueError('bitmap overrun')
  images.append((w,h,pixels))
 instances=[]
 for i in range(u(b,112)):
  p=d+u(b,116)+i*64;instances.append((u(b,p),s(b,p+12),s(b,p+14),s(b,p+28)))
 seq=[]
 for i in range(u(b,128)):
  r=d+u(b,132)+i*128;p=d+u(b,r+64);flags=u(b,p);scale=struct.unpack_from('>i',b,p+4)[0]/65536
  views,frames=struct.unpack_from('>hh',b,p+8)
  if not 0<views<=64 or not 0<frames<=256:raise ValueError('sequence dimensions')
  refs=[list(struct.unpack_from('>'+'h'*views,b,p+64+f*(46+2*views)+46)) for f in range(frames)]
  if any(v<0 or v>=len(instances) for row in refs for v in row):raise ValueError('instance reference')
  seq.append((flags,scale,refs))
 return images,instances,seq

def resolved_instance(images, instances, flags, row, view):
 # Some mirrored sequences store only the front/left/back views. Keep any
 # explicitly supplied right-facing art (e.g. asymmetric weapons) intact.
 fl,rx,ry,bi=instances[row[view]]
 if bi<0 and flags&1 and 0<view<len(row):
  source=len(row)-view
  if source<len(row):
   sf,sx,sy,sb=instances[row[source]]
   if sb>=0:
    w=images[sb][0]
    return sf^1,w-sx,sy,sb
 return fl,rx,ry,bi

def main():
 base=BASE;out=OUT;out.mkdir(parents=True,exist_ok=True)
 scene=json.loads((base/'scene.json').read_text());textures=[];quads=[]
 # Original tile order, positive x in original coordinates (no display-only reversal).
 from terrain_lighting import lighting_settings, shading_table
 collection=(base/'landscape-l1.collection').read_bytes()
 ims,_,_=decode(collection,palette_indices=True)
 settings=lighting_settings(FIXTURE)
 c=u(collection,248)+u(collection,68)
 palette=[struct.unpack_from('>3H',collection,c+32+i*8) for i in range(u(collection,c))]
 table=shading_table(palette,settings)
 mw,mh=dimensions();cols,rows=mw//32,mh//32
 land=Image.new('RGBA',(mw*8,mh*8))
 for i in tracked(range(cols*rows),'地形タイルを変換'):
  w,h,p=ims[2*i];sw,sh,shade=ims[2*i+1]
  if (w,h)!=(256,256) or (sw,sh)!=(w,h):raise ValueError('terrain light tile dimensions')
  lit=b''.join(table[shade[j]][p[j]] for j in range(0,len(p),4))
  land.paste(Image.frombytes('RGBA',(w,h),lit),((i%cols)*256,(i//cols)*256))
 textures.append((land.width,land.height,land.tobytes()));land.resize((896,768)).save(out/'terrain-preview.png')
 stats=[];indexed=[(0,0,b'')]
 for a in tracked(scene['assets'],'スプライト画像を変換'):
  raw=(base/a['file']).read_bytes();ims,instances,seq=decode(raw);start=len(textures);textures.extend(ims)
  for iw,ih,ip in decode(raw,palette_indices=True)[0]:
   pairs=bytearray(iw*ih*2);pairs[0::2]=ip[0::4];pairs[1::2]=ip[3::4];indexed.append((iw,ih,pairs))
  for si,(flags,scale,refs) in enumerate(seq):
   for fi,row in enumerate(refs):
    for vi,ii in enumerate(row):
     fl,rx,ry,bi=resolved_instance(ims,instances,flags,row,vi)
     if bi<0:continue
     if bi>=len(ims):raise ValueError('bitmap reference')
     w,h,_=ims[bi]
     if [a['runtime_index'],si] in scene.get('axial_candidates',[]) and len(row)==1 and fl&16 and w>=h*3:fl|=0x40000000
     quads.append((a['runtime_index'],si,fi,vi,len(row),flags,fl,start+bi,float(rx),float(ry),scale,float(w),float(h)))
  stats.append({'collection':a['runtime_index'],'bitmaps':len(ims),'sequences':len(seq)})
 with (out/'scene.pack').open('wb') as f:
  f.write(b'MYTHVR01');f.write(struct.pack('<4I',mw,mh,len(textures),len(quads)))
  f.write((base/'terrain-heights.i16le').read_bytes())
  for w,h,p in textures:f.write(struct.pack('<II',w,h));f.write(p)
  for q in quads:f.write(struct.pack('<8I5f',*q))
 with (out/'sprite-indices.pack').open('wb') as f:
  f.write(b'MYTHIX01');f.write(struct.pack('<I',len(indexed)))
  for w,h,p in indexed:f.write(struct.pack('<II',w,h));f.write(p)
 (out/'snapshot.bin').write_bytes((ROOT/'research/fixtures/bridge/paused-0000.bin').read_bytes())
 result={'textures':len(textures),'direction_frames':len(quads),'bytes':(out/'scene.pack').stat().st_size,'collections':stats,'terrain_lighting':settings,'lighting_method':'original 256-level integer shade table, baked into terrain RGB; alpha opaque'}
 (REPORT/'vr-assets.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
if __name__=='__main__':main()
