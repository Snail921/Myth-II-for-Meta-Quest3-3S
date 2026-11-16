"""Original media triangles, with bounded reflection proxies for sloping rivers."""
from pathlib import Path
import struct,json,collections
ROOT=Path(__file__).resolve().parents[1]
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions
def main():
 b=(BASE/'terrain-cells.bin').read_bytes();w,h=dimensions()
 assert len(b)==w*h*12
 planes=collections.defaultdict(list)
 for y in range(h-1):
  for x in range(w-1):
   at=(y*w+x)*12;flags=b[at+5]
   if not flags&32 or flags&64:continue
   z=struct.unpack_from('<h',b,at+8)[0]/512
   a=(x,y,z);c=(x,y+1,z);d=(x+1,y+1,z);e=(x+1,y,z)
   triangles=((a,e,c),(e,d,c)) if (x^y)&1 else ((a,e,d),(a,d,c))
   for i,t in enumerate(triangles):
    if flags&(8<<i):planes[z].extend(t)
 original_levels=len(planes);grouped=original_levels>4;error=0
 if grouped:
  # Weighted farthest-point anchors retain the largest real level first; vertex
  # heights stay exact. Only the mirror transform uses a representative height.
  anchors=[max(planes,key=lambda z:len(planes[z]))]
  while len(anchors)<4:
   anchors.append(max((z for z in planes if z not in anchors),key=lambda z:len(planes[z])*min(abs(z-a) for a in anchors)**2))
  merged=collections.defaultdict(list)
  for z,vertices in planes.items():
   nearest=min(anchors,key=lambda a:abs(a-z));merged[nearest].extend(vertices);error=max(error,abs(nearest-z))
  assert sum(map(len,merged.values()))==sum(map(len,planes.values()));planes=merged
 out=OUT/'reflection-water.pack'
 out.write_bytes((b'MYTHRF03' if grouped else b'MYTHRF02')+struct.pack('<I',len(planes))+b''.join(struct.pack('<fI',level,len(vertices))+b''.join(struct.pack('<3f',*v) for v in vertices) for level,vertices in sorted(planes.items())))
 result={'original_levels':original_levels,'grouped':grouped,'maximum_mirror_height_error':error,'planes':{str(level):len(vertices)//3 for level,vertices in planes.items()},'bytes':out.stat().st_size,'source':'original media flags and cell water height'}
 (REPORT/'reflection-water-assets.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(result)
if __name__=='__main__':main()
