"""Read-only, bounded access to the authorized Myth II process in debug Winlator."""
import subprocess,struct,re
ADB='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
SERIAL='340YC10G8T047C'
class QuestMemory:
 def __init__(self):
  self.base=[ADB,'-s',SERIAL,'shell']
  ps=self.shell('ps -A')
  pids=[int(s.split()[1]) for s in ps.splitlines() if s.endswith('Myth II.exe')]
  if len(pids)!=1: raise ValueError('Expected exactly one original game process')
  self.pid=pids[0]
  maps=self.shell(f'run-as com.winlator cat /proc/{self.pid}/maps')
  self.ranges=[]
  for line in maps.splitlines():
   address,permissions,*_=line.split()
   if permissions.startswith('r'):
    a,b=address.split('-');self.ranges.append((int(a,16),int(b,16)))
  if self.read(0x400000,2)!=b'MZ': raise ValueError('Expected original PE address missing')
 def shell(self,command):
  result=subprocess.run(self.base+[command],capture_output=True,text=True,timeout=20,check=True)
  if result.stderr.strip(): raise RuntimeError(result.stderr)
  return result.stdout
 def read(self,address,size):
  if not 0<size<=2097152: raise ValueError('Read size outside bound')
  if not any(a<=address and address+size<=b for a,b in self.ranges): raise ValueError(f'Unmapped read {address:x}+{size:x}')
  cmd=f'run-as com.winlator sh -c "dd if=/proc/{self.pid}/mem bs={size} count=1 skip={address} iflag=skip_bytes status=none | od -An -v -tx1"'
  data=bytes.fromhex(self.shell(cmd))
  if len(data)!=size: raise ValueError('Short process read')
  return data
 def u32(self,address): return struct.unpack('<I',self.read(address,4))[0]
if __name__=='__main__':
 m=QuestMemory()
 for a in [0x529f20,0x52a9c0]:
  b=m.read(a,64);print(hex(a),b.hex(),struct.unpack_from('<h',b,24),struct.unpack_from('<I',b,32),struct.unpack_from('<hhhI',b,44))
 actors=m.u32(0x529f54);objects=m.u32(0x52a9f4)
 b=m.read(actors,216*80)
 for i in range(80):
  record=b[i*216:(i+1)*216];ident=struct.unpack_from('<H',record)[0]
  if ident==10359:
   obj=struct.unpack_from('<h',record,12)[0];o=m.read(objects+obj*80,80)
   print('Darras candidate',i,ident,'object',obj, 'object bytes',o.hex(),'position',struct.unpack_from('<iii',o,8),'yaw',struct.unpack_from('<H',o,28)[0])
