"""Read-only X11 window capture through the installed debug app's own socket."""
import subprocess,struct,json,zlib,threading
from pathlib import Path
ADB='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
p=subprocess.Popen([ADB,'-s','340YC10G8T047C','shell','-T','run-as','com.winlator','toybox','nc','-U','/data/data/com.winlator/files/rootfs/tmp/.X11-unix/X0'],stdin=subprocess.PIPE,stdout=subprocess.PIPE)
timeout=threading.Timer(20,p.terminate)
timeout.start()
pending=b''
carry=b''
def read(n):
 global pending,carry
 while len(pending)<n:
  raw=p.stdout.read1(65536)
  if not raw: raise RuntimeError('X socket closed')
  raw=carry+raw;carry=b''
  if raw.endswith(b'\r'): raw,carry=raw[:-1],b'\r'
  # Observed CRLF expansion in this host's ADB shell transport.
  pending+=raw.replace(b'\r\n',b'\n')
 result,pending=pending[:n],pending[n:]
 return result
def send(b): p.stdin.write(b);p.stdin.flush()
def request(b):
 send(b)
 while True:
  h=read(32)
  if h[0]==0: raise RuntimeError('X error '+h.hex())
  if h[0]==1: return h,read(struct.unpack_from('<I',h,4)[0]*4)
def png(path,w,h,b):
 def chunk(t,d): return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d))
 rows=[]
 for y in range(h):
  line=b[y*w*4:(y+1)*w*4]; rgb=bytearray(w*3)
  rgb[0::3]=line[2::4];rgb[1::3]=line[1::4];rgb[2::3]=line[0::4]
  rows.append(b'\x00'+rgb)
 Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(rows)))+chunk(b'IEND',b''))
try:
 send(struct.pack('<BBHHHHH',108,0,11,0,0,0,0))
 h=read(8);d=read(struct.unpack_from('<H',h,6)[0]*4)
 if h[0]!=1: raise RuntimeError(repr(d))
 vendor=struct.unpack_from('<H',d,16)[0]; off=32+((vendor+3)//4)*4+d[21]*8
 root=struct.unpack_from('<I',d,off)[0]
 q,_=request(struct.pack('<BBHI',38,0,2,root))
 print('pointer',struct.unpack_from('<hhhhH',q,16))
 out=Path('research/reports/quest-x11');out.mkdir(exist_ok=True)
 todo=[root]; seen=set(); rows=[]
 while todo:
  wid=todo.pop(0)
  if wid in seen: continue
  seen.add(wid)
  try: h,d=request(struct.pack('<BBHI',15,0,2,wid))
  except RuntimeError: continue
  count=struct.unpack_from('<H',h,16)[0]
  todo.extend(struct.unpack('<'+'I'*count,d[:count*4]))
  try: h,d=request(struct.pack('<BBHI',14,0,2,wid))
  except RuntimeError: continue
  x,y,w,ht=struct.unpack_from('<hhHH',h,12)
  row={'id':wid,'x':x,'y':y,'width':w,'height':ht,'depth':h[1]}
  if wid != root and w>=300 and ht>=200:
   try:
    h,b=request(struct.pack('<BBHIhhHHI',73,2,5,wid,0,0,w,ht,0xffffffff))
    if len(b)==w*ht*4:
     path=out/f'window-{wid}.png';png(path,w,ht,b);row['image']=str(path)
   except RuntimeError as e: row['error']=str(e)
  rows.append(row)
 (out/'windows.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
 print(json.dumps([r for r in rows if 'image' in r]))
finally:
 timeout.cancel()
 p.terminate()
