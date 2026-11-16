"""Decode versioned bridge snapshots into pointer-free scene instances."""
import struct,json,math,sys
from pathlib import Path
def decode(data):
 if len(data)<64 or data[:8]!=b'MYTHBR01': raise ValueError('Bad bridge magic')
 h=struct.unpack_from('<14I',data,8)
 version,tick,endtick,rng,ac,oc,astride,ostride,*_=h
 if version not in (1,2,3,4,5,6,7,8) or tick!=endtick or astride!=216 or ostride!=80 or ac>400 or oc>8192: raise ValueError('Invalid schema/header')
 raw_size=64+ac*216+oc*80
 expected=raw_size+(272 if version>=2 else 0);palettes=[]
 if version>=3:
  if len(data)<expected+4:raise ValueError('Missing palette count')
  count=struct.unpack_from('<I',data,expected)[0]
  if count>128:raise ValueError('Invalid palette count')
  if len(data)<expected+4+count*1028:raise ValueError('Invalid palette payload length')
  for i in range(count):
   at=expected+4+i*1028;key=struct.unpack_from('<I',data,at)[0]
   if (key&255)==255 or (key>>8)>=128:raise ValueError('Invalid palette key')
   palettes.append({'collection':key&255,'index':key>>8,'rgba_hex':data[at+4:at+1028].hex()})
  expected+=4+count*1028
 models=None
 if version>=4:
  if len(data)<expected+4:raise ValueError('Missing model count')
  count=struct.unpack_from('<I',data,expected)[0];expected+=4
  if count!=0xffffffff:
   if count>512 or len(data)<expected+count*36:raise ValueError('Invalid model payload')
   models=[]
   for i in range(count):
    vals=struct.unpack_from('<I3i9hH',data,expected+i*36)
    models.append(dict(geometry=vals[0],position_raw=vals[1:4],matrix=vals[4:13],owner=vals[13]))
   expected+=count*36
 surface=None
 if version>=5:
  if len(data)<expected+12:raise ValueError('Missing terrain header')
  epoch,count=struct.unpack_from('<QI',data,expected);expected+=12
  if count>4 or (not epoch and count) or len(data)<expected+count*131076:raise ValueError('Invalid terrain payload')
  tiles=[]
  for i in range(count):
   tile=struct.unpack_from('<I',data,expected+i*131076)[0]
   if tile>=42 or tile in tiles:raise ValueError('Invalid terrain tile')
   tiles.append(tile)
  expected+=count*131076;surface=dict(epoch=epoch,tiles=tiles)
 lights=None
 if version>=6:
  if len(data)<expected+4:raise ValueError('Missing light header')
  size=struct.unpack_from('<I',data,expected)[0];expected+=4;end=expected+size
  if size<4 or size>262144 or end>len(data):raise ValueError('Invalid light size')
  count=struct.unpack_from('<I',data,expected)[0];expected+=4;lights=[]
  if count>256:raise ValueError('Invalid light count')
  for _ in range(count):
   if end-expected<64:raise ValueError('Short light record')
   v=struct.unpack_from('<16I',data,expected);expected+=64
   if v[1] not in (1,2,3) or not 0<v[8]<=256 or not 0<v[9]<=256 or v[10]>65535 or v[13]>2:raise ValueError('Invalid light record')
   expected+=v[8]*v[9]
   if expected>end:raise ValueError('Short light pixels')
   lights.append(dict(source=v[0],domain=v[1],key=v[2],width=v[8],height=v[9],alpha=v[10]))
  if expected!=end:raise ValueError('Light trailing bytes')
 particles=None
 if version>=7:
  if len(data)<expected+4:raise ValueError('Missing particle header')
  count=struct.unpack_from('<I',data,expected)[0];expected+=4
  if count>2048 or len(data)<expected+count*64:raise ValueError('Invalid particle count')
  particles=[]
  for i in range(count):
   v=struct.unpack_from('<16I',data,expected+i*64)
   if v[2]>255 or v[3]>255 or not 0<v[8]<=32767 or v[13]>=255 or v[15]>3:raise ValueError('Invalid particle')
   if v[15]==3:
    if v[7]>65535 or not 0<v[9]<=32767 or not 3<=v[10]<=128 or v[11] or v[12] or v[14]>=128:raise ValueError('Invalid mesh-effect ring')
   elif v[15]==2:
    if not 0<v[9]<=512 or (v[7]&65535)>=(v[7]>>16):raise ValueError('Invalid weather')
   elif v[7]>65535 or not 0<v[9]<=32767 or v[10]>3:raise ValueError('Invalid particle')
   particles.append(dict(source=v[0],signature=v[1],sequence=v[2],frame=v[3],kind=v[15],alpha=v[7] if v[15]!=2 else None,scale_x=v[8],scale_y=v[9] if v[15]!=2 else None))
  expected+=count*64
 if len(data)!=expected: raise ValueError('Invalid payload length')
 selection=set();player=team=None
 if version>=2:
  player,team,mode,flags,count=struct.unpack_from('<iiIII',data,raw_size)
  if count>126:raise ValueError('Invalid selection count')
  selection=set(struct.unpack_from('<'+'h'*count,data,raw_size+20))
  if any(i<0 or i>=ac for i in selection):raise ValueError('Invalid selection slot')
 actors=data[64:64+ac*216];objects=data[64+ac*216:];owners={}
 for i in range(ac):
  a=actors[i*216:(i+1)*216];ident=struct.unpack_from('<H',a)[0]
  if not ident: continue
  idx=struct.unpack_from('<h',a,12)[0]
  if idx<0 or idx>=oc: raise ValueError('Actor object index out of bounds')
  owners[idx]={'actor_slot':i,'unit_id':ident,'monster_definition_index':struct.unpack_from('<h',a,2)[0],'unit_definition_index':struct.unpack_from('<h',a,20)[0],'team_raw':a[22],'owner_raw':a[23]}
  if version>=2:owners[idx].update(selected=i in selection,controllable=a[22]==team and a[23]==player and not a[7]&0x10)
 result=[]
 for i in range(oc):
  b=objects[i*80:(i+1)*80];ident=struct.unpack_from('<H',b)[0]
  if not ident:continue
  xyz=struct.unpack_from('<iii',b,8);yaw=struct.unpack_from('<H',b,28)[0]
  instance={'object_slot':i,'object_id':ident,'flags':struct.unpack_from('<I',b,4)[0],'position_raw':list(xyz),'position_game_units':[v/512 for v in xyz],'yaw_raw':yaw,'yaw_turns':yaw/65536,'scale_raw':struct.unpack_from('<h',b,2)[0],'object_kind':b[40],'owner_index':struct.unpack_from('<h',b,38)[0],'collection_index':b[42],'sequence_index':b[43],'palette_index':b[44],'frame_index':b[46],'frame_tick':b[47],'ticks_per_frame':b[48],'view_variant':b[49],'definition_index':struct.unpack_from('<h',b,64)[0]}
  if i in owners:instance.update(owners[i])
  result.append(instance)
 return {'particles':particles,'lights':lights,'persistent_surface':surface,'animated_models':models,'palettes':palettes,'schema':'myth.scene.v1','tick':tick,'primary_rng':rng,'capture_us':(h[8]+(h[9]<<32))/1000,'coordinates':'original x/y horizontal, z up; 512 raw units per game unit; not metres','coherence':'tick/RNG/pool headers bracketed; not an atomic snapshot guarantee','instances':result}
if __name__=='__main__':
 p=Path(sys.argv[1]);out=Path(sys.argv[2]);r=decode(p.read_bytes());out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');print(json.dumps({'tick':r['tick'],'instances':len(r['instances']),'units':sum('unit_id' in x for x in r['instances'])}))
