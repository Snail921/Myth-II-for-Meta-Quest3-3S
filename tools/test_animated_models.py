"""Compare dynamic transform arithmetic against original x86 instructions."""
from verify_combat_builders import *
import random
c=machine();r=random.Random(1337)
for i in range(1000):
 matrix=[512 if j in (0,4,8) else 0 for j in range(9)] if i==0 else [r.randrange(-700,701) for _ in range(9)]
 v=[r.randrange(-5000,5001) for _ in range(3)];center=[r.randrange(-2000,2001) for _ in range(3)];pos=[r.randrange(-100000,100001) for _ in range(3)]
 c.mem_write(0x100e000,struct.pack('<9h',*matrix));c.mem_write(0x100e100,struct.pack('<3h',*v));c.mem_write(0x100e110,struct.pack('<3h',*center));c.mem_write(0x100e120,struct.pack('<3i',*pos))
 run(c,0x42cfee,[0x100e120,0x100e140],{UC_X86_REG_EAX:0x100e100,UC_X86_REG_ECX:0x100e000,UC_X86_REG_EDX:0x100e110})
 actual=struct.unpack('<3i',c.mem_read(0x100e140,12))
 expected=tuple(pos[k]+(((sum(matrix[k+j*3]*(v[j]-center[j]) for j in range(3))>>9)+32768)%65536-32768) for k in range(3))
 assert actual==expected,(actual,expected)
print('PASS 1000 model transforms against original x86 instructions')
from decode_bridge import decode
for version in (1,2,3,4):
 data=b'MYTHBR01'+struct.pack('<14I',version,1,1,0,0,0,216,80,0,0,0,0,0,0)
 if version>=2:data+=bytes(272)
 if version>=3:data+=bytes(4)
 if version==4:
  prefix=data
  data+=struct.pack('<I',1)+struct.pack('<I3i9hH',0x30316267,512,1024,1536,*([512,0,0,0,512,0,0,0,512]),1)
 result=decode(data)
 if version==4:
  assert result['animated_models'][0]['geometry']==0x30316267
  assert decode(prefix+struct.pack('<I',0xffffffff))['animated_models'] is None
  assert decode(prefix+bytes(4))['animated_models']==[]
  for bad in (data[:-1],data+b'x',prefix+struct.pack('<I',513)):
   try:decode(bad)
   except ValueError:pass
   else:raise AssertionError('Malformed accepted')
print('PASS snapshot v1-v4, models, missing/oversized payload, unavailable/empty pool')
