"""Read-only Windows process snapshot for exact-hash Myth engine profiles. Never injects commands."""
import argparse,ctypes as C,struct,json,time
from pathlib import Path
from engine_profile import load_profile,parse_pool
from ctypes import wintypes as W
k=C.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE
k.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)];k.ReadProcessMemory.restype=W.BOOL
k.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)];k.QueryFullProcessImageNameW.restype=W.BOOL
k.CloseHandle.argtypes=[W.HANDLE]
class Memory:
 def __init__(self,pid):
  self.h=k.OpenProcess(0x1010,False,pid) # query limited + VM_READ only
  if not self.h:raise C.WinError(C.get_last_error())
  name=C.create_unicode_buffer(32768);size=W.DWORD(len(name))
  if not k.QueryFullProcessImageNameW(self.h,0,name,C.byref(size)):raise C.WinError(C.get_last_error())
  self.exe=Path(name.value)
  try:self.profile=load_profile(self.exe)
  except Exception:self.close();raise
  if self.read(0x400000,2)!=b'MZ':raise ValueError('Unexpected image base')
 def read(self,addr,size):
  if not 0x10000<=addr<0xffffffff or not 0<size<=2*1024*1024:raise ValueError('read bounds')
  data=C.create_string_buffer(size);n=C.c_size_t()
  if not k.ReadProcessMemory(self.h,addr,data,size,C.byref(n)) or n.value!=size:raise C.WinError(C.get_last_error())
  return data.raw
 def u32(self,addr):return struct.unpack('<I',self.read(addr,4))[0]
 def close(self):
  if self.h:k.CloseHandle(self.h);self.h=None
 def snapshot(self):
  a=self.profile['addresses'];start=self.u32(a['tick']);state=self.u32(a['game_state'])&65535
  result={'profile':self.profile['id'],'state':state,'tick':start,'rng':self.u32(a['rng']),'mesh_pointer':self.u32(a['mesh'])}
  if state!=3:return result
  mesh=result['mesh_pointer'];header=self.read(mesh,1024);w,h=struct.unpack_from('<HH',header,8);
  if not (0<w<=16 and 0<h<=16):raise ValueError('Invalid map dimensions')
  result['map']={'id_raw':header[:4].hex(),'width':w*32,'height':h*32}
  pools={}
  for name,prefix,limit,stride in [('actors','actor_pool',400,self.profile['record_sizes']['actor']),('objects','object_pool',8192,self.profile['record_sizes']['object'])]:
   raw=self.read(a[prefix],64);pool=parse_pool(raw,self.profile['pool_layout']);result[name]=pool
   if not pool['name'].startswith('monster' if name=='actors' else 'object'):raise ValueError('Invalid pool name: '+str(pool))
   if pool['stride']!=stride or pool['count']>limit:raise ValueError('Invalid pool layout: '+str(pool))
   data=self.read(pool['data'],pool['count']*stride) if pool['count'] else b''
   result[name]['active_slots']=sum(struct.unpack_from('<H',data,i*stride)[0]!=0 for i in range(pool['count']))
   result[name]['sample_hex']=data[:stride*3].hex()
   pools[name]=data
  selection=self.read(a['selection'],256);n,flags=struct.unpack_from('<HH',selection)
  if n>126:raise ValueError('Invalid selection count')
  slots=list(struct.unpack_from('<'+'h'*n,selection,4))
  if not all(0<=s<result['actors']['count'] for s in slots):raise ValueError('Invalid selected slot')
  units=[]
  stride=self.profile['record_sizes']['actor']
  for slot in range(result['actors']['count']):
   actor=pools['actors'][slot*stride:(slot+1)*stride];identity=struct.unpack_from('<H',actor)[0]
   if not identity:continue
   obj=struct.unpack_from('<h',actor,12)[0]
   if not 0<=obj<result['objects']['count']:continue
   o=pools['objects'][obj*80:(obj+1)*80];xyz=struct.unpack_from('<iii',o,8)
   units.append({'slot':slot,'identity':identity,'definition':struct.unpack_from('<h',actor,2)[0],'object_slot':obj,'position':[v/512 for v in xyz],'team_byte':actor[22],'player_byte':actor[23]})
  result['units']=units
  result['selection']={'count':n,'flags':flags,'slots':slots};result['player']=self.u32(a['player']);result['team']=self.u32(a['team'])
  result['end_tick']=self.u32(a['tick']);result['consistent_tick']=result['end_tick']==start
  result['same_mesh']=self.u32(a['mesh'])==mesh
  return result
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('pid',type=int);ap.add_argument('--samples',type=int,default=3);ap.add_argument('--out',required=True);args=ap.parse_args();m=Memory(args.pid)
 try:
  result={'executable':str(m.exe),'profile':m.profile['id'],'read_only':True,'samples':[]}
  for _ in range(args.samples):
   try:s=m.snapshot()
   except Exception as e:s={'error':str(e)}
   result['samples'].append(s);print(json.dumps({key:value for key,value in s.items() if key not in ('actors','objects','units')}),flush=True);time.sleep(.2)
  Path(args.out).write_text(json.dumps(result,indent=2))
 finally:m.close()
