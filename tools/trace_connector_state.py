"""Read-only connector state trace for the original 471b process."""
import sys,time,json,struct
from pathlib import Path
from probe_engine_state import Memory
m=Memory(int(sys.argv[1]));end=time.monotonic()+600;last=None
out=Path('research/reports/fence-connector-live.jsonl')
with out.open('w',encoding='utf8') as f:
 while time.monotonic()<end:
  try:
   h=m.read(0x52ecbc,64);n=struct.unpack_from('<H',h,42)[0];stride=struct.unpack_from('<H',h,24)[0];ptr=struct.unpack_from('<I',h,48)[0]
   if 0<n<=512 and stride==512 and 0x10000<=ptr<0x7fffffff:
    data=m.read(ptr,n*stride);tick=m.u32(0x6bee64)
    if data!=last:
     rows=[]
     for i in range(n):
      r=data[i*512:(i+1)*512];ident,kind,col,count=struct.unpack_from('<4H',r)
      if ident and count<=31:rows.append({'slot':i,'id':ident,'kind':kind,'col':col,'nodes':[struct.unpack_from('<Ihhii',r,8+j*16) for j in range(count)]})
     f.write(json.dumps({'tick':tick,'chains':rows})+'\n');f.flush();last=data
  except Exception as e:
   last=None;time.sleep(.2);continue
  time.sleep(.05)
