from build_progress import tracked
"""Original terrain indices/shades/LUT and frame-specific ground shadow masks."""
import json,struct
from pathlib import Path
from prepare_vr_assets import ROOT,decode,u,s
from terrain_lighting import lighting_settings,shading_table
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions
from inspect_surface_effects import inspect

def main():
 base=BASE;out=OUT;scene=json.loads((base/'scene.json').read_text(encoding='utf-8'))
 b=(base/'landscape-l1.collection').read_bytes();ims=decode(b,palette_indices=True)[0]
 mw,mh=dimensions();cols,rows=mw//32,mh//32;w,h=mw*8,mh*8;indices=bytearray(w*h);shade=bytearray(w*h)
 for i in range(cols*rows):
  for target,image in ((indices,ims[i*2]),(shade,ims[i*2+1])):
   assert image[:2]==(256,256)
   raw=image[2][0::4]
   for y in range(256):
    at=((i//cols)*256+y)*w+(i%cols)*256;target[at:at+256]=raw[y*256:y*256+256]
 c=u(b,248)+u(b,68);assert u(b,c)==256
 table=shading_table([struct.unpack_from('>3H',b,c+32+i*8) for i in range(256)],lighting_settings(FIXTURE))
 lut=b''.join(b''.join(row) for row in table)
 # Exact zero-effect equivalence to the currently shipped terrain RGBA.
 pack=(out/'scene.pack').read_bytes();at=24+mw*mh*2;assert struct.unpack_from('<2I',pack,at)==(w,h)
 rgba=b''.join(table[sh][ix] for ix,sh in zip(indices,shade));assert pack[at+8:at+8+len(rgba)]==rgba
 masks={};frames=[]
 for a in tracked(scene['assets'],'表面効果を変換'):
  b=(base/a['file']).read_bytes();meta=inspect(b);images=decode(b,palette_indices=True)[0];col=a['runtime_index']
  for m in meta['maps']:
   if m['bitmap']<0:continue
   if m['flags']&~1:continue # Unsupported auxiliary FX maps are not interpreted as ground shadows.
   mw,mh,pixels=images[m['bitmap']];key=(col<<16)|m['index']
   masks[key]=(0 if m['kind']=='shadow' else 1,-(m['registration_raw'][0]>>10),-(m['registration_raw'][1]>>10),mw,mh,bytes(pixels[0::4]))
  for seq in meta['sequences']:
   for f,idx in enumerate(seq['maps']):
    key=(col<<16)|idx
    if idx>=0 and key in masks:frames.append((col,seq['sequence'],f,key))
 from tag_sources import inventory
 tags=inventory()
 def get(kind,name):
  path,t=tags[kind,name]
  with (ROOT/path).open('rb') as f:f.seek(t['offset']);return f.read(t['size'])
 for rec in json.loads((REPORT/'vr-models.json').read_text(encoding='utf-8')):
  if not rec['animated_first_pose_only']:continue
  b=get('anim',rec['tag']);n=s(b,4);mw,mh,stride=struct.unpack_from('>3h',b,0x3b4);ox,oy=struct.unpack_from('>2h',b,0x3bc);size=u(b,0x3f8)
  if not size:continue
  start=u(b,0x3f4)
  assert 0<mw<=stride and size==n*mh*stride and start>=1024 and start+size<=len(b)
  for i in range(n):
   mode=get('mode',b[12+i*16:16+i*16].decode('mac_roman'));key=u(mode,4)
   pixels=b''.join(b[start+(i*mh+y)*stride:start+(i*mh+y)*stride+mw] for y in range(mh))
   entry=(2,ox,oy,mw,mh,pixels)
   if key in masks and masks[key]!=entry:raise ValueError('Geometry reused with different shadow state: need explicit anim state IPC')
   masks[key]=entry
 with (out/'surface-effects.pack').open('wb') as f:
  f.write(b'MYTHSF01');f.write(struct.pack('<4I',w,h,len(masks),len(frames)));f.write(indices);f.write(shade);f.write(lut)
  for key,(kind,ox,oy,mw,mh,pixels) in masks.items():f.write(struct.pack('<2I2i2I',key,kind,ox,oy,mw,mh));f.write(pixels)
  for frame in frames:f.write(struct.pack('<4I',*frame))
 report=dict(width=w,height=h,masks=len(masks),frames=len(frames),bytes=(out/'surface-effects.pack').stat().st_size,zero_effect_rgba_exact=True,model_shadow_key='unique geometry tag in current map; validated during export')
 (REPORT/'surface-assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(report)
if __name__=='__main__':main()
