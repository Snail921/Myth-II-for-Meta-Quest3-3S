"""Resolve the paused scene's runtime collections to original asset bodies."""
import json,struct,hashlib
from pathlib import Path
from quest_memory import QuestMemory
from decode_bridge import decode
ROOT=Path(__file__).resolve().parents[1]
def u32(b,o,e='<'):return struct.unpack_from(e+'I',b,o)[0]
def key(b,e):return tuple(u32(b,o,e) for o in [64,96,112,128,252])
def main():
 out=ROOT/'build/scene-bridge';out.mkdir(exist_ok=True)
 scene=decode((ROOT/'research/fixtures/bridge/paused-0000.bin').read_bytes())
 m=QuestMemory();tick=m.u32(0x67ec24)
 if tick!=scene['tick']:raise ValueError('Export static references while paused at the captured tick')
 inv=json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf-8'));lookup={};landscape=[]
 for archive in inv:
  for tag in archive.get('tags',[]):
   if tag['type']!='.256' or tag['size']<320:continue
   with (ROOT/archive['path']).open('rb') as f:f.seek(tag['offset']);head=f.read(320)
   entry=(archive['path'],tag,head);lookup.setdefault(key(head,'>'),[]).append(entry)
   if tag.get('id_hex')=='6c312020':landscape.append(entry)
 colbase=m.u32(0x52b4d4);assets=[];seqs={}
 for i in sorted(set(x['collection_index'] for x in scene['instances'])-{255}):
  head=m.read(colbase+i*320,320);count=u32(head,128)
  if count>1024:raise ValueError('Unexpected sequence count')
  refs=m.read(u32(head,140),count*128) if count else b''
  candidates=[]
  for archive,tag,original_head in lookup.get(key(head,'<'),[]):
   with (ROOT/archive).open('rb') as f:
    f.seek(tag['offset']);raw=f.read(tag['size'])
   start=u32(raw,248,'>')+u32(raw,132,'>')
   source_refs=raw[start:start+count*128]
   if len(source_refs)!=len(refs):continue
   if all(refs[n:n+64].lower()==source_refs[n:n+64].lower() and struct.unpack_from('<II',refs,n+64)==struct.unpack_from('>II',source_refs,n+64) for n in range(0,len(refs),128)):
    candidates.append((archive,tag,raw))
  hashes={hashlib.sha256(r).hexdigest() for a,t,r in candidates}
  if len(hashes)!=1:raise ValueError(f'Ambiguous collection {i}: {len(candidates)} matches')
  archive,tag,raw=candidates[0];filename=f'collection-{i:03d}-{tag["id_hex"]}.bin';(out/filename).write_bytes(raw)
  assets.append({'runtime_index':i,'tag_id_hex':tag['id_hex'],'name':tag['name'],'file':filename,'sha256':next(iter(hashes)),'archive':archive,'offset':tag['offset'],'size':len(raw),'sequence_refs_verified':count})
  for n in set(x['sequence_index'] for x in scene['instances'] if x['collection_index']==i):
   if n>=count:continue
   ref=refs[n*128:(n+1)*128];ptr=u32(ref,72);h=m.read(ptr,64)
   name=ref[:64].split(b'\0')[0].decode('mac_roman');flags=u32(h,0);views,frames,tpf=struct.unpack_from('<hhh',h,8)
   if not 0<views<=64 or not 0<frames<=256:raise ValueError('Invalid animation dimensions')
   frames_raw=m.read(ptr+64,frames*(46+2*views))
   seqs[f'{i}:{n}']={'name':name,'flags':flags,'views':views,'frames':frames,'ticks_per_frame':tpf,'pixels_to_world_raw':struct.unpack_from('<i',h,4)[0],'view_instance_indices':[list(struct.unpack_from('<'+'h'*views,frames_raw,f*(46+2*views)+46)) for f in range(frames)]}
 mh=m.read(m.u32(0x529bd4),1024);w,h=struct.unpack_from('<HH',mh,8);w*=32;h*=32
 if (w,h)!=(224,192):raise ValueError('Unexpected map')
 cells=m.read(u32(mh,20),w*h*12);heights=struct.unpack('<'+'h'*(w*h),b''.join(cells[n:n+2] for n in range(0,len(cells),12)))
 fixture=json.loads((ROOT/'research/fixtures/maps/willow-creek/decoded-map.json').read_text(encoding='utf-8'))
 original=(ROOT/'research/fixtures/maps/willow-creek/mesh.bin').read_bytes();off=fixture['sections']['mesh']['offset'];original_heights=[struct.unpack_from('>h',original,off+i*12)[0] for i in range(w*h)]
 if list(heights)!=original_heights:raise ValueError('Live terrain heights differ from fixture')
 (out/'terrain-heights.i16le').write_bytes(struct.pack('<'+'h'*len(heights),*heights));(out/'terrain-cells.bin').write_bytes(cells)
 # Preserve the original landscape texture collection, without inventing its format.
 if len(landscape)!=1:raise ValueError('Ambiguous landscape asset')
 a,t,_=landscape[0]
 with (ROOT/a).open('rb') as f:f.seek(t['offset']);raw=f.read(t['size'])
 (out/'landscape-l1.collection').write_bytes(raw)
 if m.u32(0x67ec24)!=tick:raise ValueError('Game advanced during static export')
 scene.update({'assets':assets,'animations':seqs,'terrain':{'width':w,'height':h,'heights_file':'terrain-heights.i16le','cells_file':'terrain-cells.bin','height_scale':1/512,'original_height_comparisons':len(heights),'landscape_file':'landscape-l1.collection'},'scope':'Data bridge proof; not an immersive VR renderer. Sprite assets remain original collection files.'})
 (out/'scene.json').write_text(json.dumps(scene,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({'assets':len(assets),'animation_states':len(seqs),'height_comparisons':len(heights),'instances':len(scene['instances'])}))
if __name__=='__main__':main()
