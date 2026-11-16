"""Original overhead-map bitmap, kept separate from the 3D landscape textures."""
import struct
from pathlib import Path
from level_assets import ROOT,inventory,raw_tag,parse_mesh,schemas
from campaign_levels import campaign
from prepare_vr_assets import decode,resolved_instance

def build_minimap(tags,mapid,target):
 mesh=parse_mesh(raw_tag(tags,'mesh',mapid),schemas())
 tag=mesh['header']['overhead_map_collection_tag'].encode('latin1').decode('mac_roman')
 images,instances,sequences=decode(raw_tag(tags,'.256',tag))
 if not sequences:raise ValueError('Missing overhead sequence')
 flags,_,refs=sequences[0]
 fl,_,_,index=resolved_instance(images,instances,flags,refs[0],0)
 if not 0<=index<len(images):raise ValueError('Missing overhead bitmap')
 w,h,pixels=images[index]
 if fl&1:
  pixels=b''.join(pixels[(y*w+x)*4:(y*w+x+1)*4] for y in range(h) for x in reversed(range(w)))
 target.mkdir(parents=True,exist_ok=True)
 (target/'minimap.rgba').write_bytes(struct.pack('<II',w,h)+pixels)
 return w,h

if __name__=='__main__':
 tags=inventory()
 for mapid,name in campaign(tags):
  key=mapid.encode('latin1').hex()
  size=build_minimap(tags,mapid,ROOT/'build/vr/assets/levels'/key)
  print(key,size,flush=True)
