"""Compare both original movement builders across VR-used option combinations."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as U
from unicorn.x86_const import *
PE={v:pefile.PE('research/extracted/update-'+v+'/Myth II.exe') for v in ('1.8.1','1.8.5')}
def run(v,units,point_count,flags,formation,facing,adapter=False):
 p=PE[v];new=v=='1.8.5';u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
 u.mem_map(0x400000,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(0x400000,p.get_memory_mapped_image());u.mem_map(0x1000000,0x40000)
 def w(a,*values):u.mem_write(a,struct.pack('<'+'I'*len(values),*values))
 actors,slots,opts,points,queue,buf=0x1001000,0x1009000,0x1009200,0x1009300,0x1009400,0x100a000
 w(0x532188 if new else 0x529f54,actors)
 for i in range(units):
  slot=i+2;u.mem_write(actors+slot*(220 if new else 216),struct.pack('<H',10000+i));u.mem_write(slots+i*2,struct.pack('<H',slot))
 u.mem_write(opts,struct.pack('<4H',flags,formation,facing,0))
 for i in range(point_count):w(points+i*8,90000-i*1000,78000+i*500)
 w(0x532284 if new else 0x52a744,2181);w(0x540a48 if new else 0x52f4e4,queue)
 w(queue,*([0]*(6 if new else 5)),4096,buf);u.mem_write(buf,b'\xa5'*4096)
 sp,stop=0x103f000,0x1000000
 if new:w(sp,stop,0,0xffffffff,units,slots,point_count,points)
 else:w(sp,stop,0xffffffff,opts,units,point_count,points)
 u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,opts if new else slots)
 entry=0x47b31c if new else 0x446764
 if adapter:
  from verify_185_adapters import load_adapter, saved_registers, assert_registers
  entry=load_adapter(u,'movement185');w(sp,stop,opts,units,slots,point_count,points)
  saved_registers(u)
 u.emu_start(entry,stop,count=200000)
 if adapter:assert_registers(u)
 assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
 length=struct.unpack('<I',u.mem_read(queue+8,4))[0];assert 8<length<4096
 data=bytes(u.mem_read(buf,length));assert bytes(u.mem_read(buf+length,16))==b'\xa5'*16
 return data.hex()
def main():
 rows=[]
 # Plain, formation, explicit facing, circular and back-and-forth routes.
 for units in (1,4,32,126):
  for count in (1,4):
   for flags,form,facing in ((1,0,0),(8,3,0),(24,3,256),(9,3,0),(11,3,0)):
    a,b=run('1.8.1',units,count,flags,form,facing),run('1.8.5',units,count,flags,form,facing)
    assert a==b,(units,count,flags)
    rows.append({'units':units,'points':count,'flags':flags,'formation':form,'facing':facing,'bytes':len(bytes.fromhex(b))})
 Path('research/reports/version-185-movement-matrix.json').write_text(json.dumps({'result':'pass','cases':rows,'limitations':'Serialization equivalence only; assignment optimization disabled. No scheduler, pathfinding, patrol execution or live command validation.'},indent=2),encoding='utf8')
 print('PASS: 40 movement builder cases match 1.8.1, including 126 units, four waypoints, facing and patrol flags')

if __name__ == "__main__": main()
