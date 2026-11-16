"""Differential formation execution, isolated PE images only; never writes to game."""
import sys, struct, math, json
from pathlib import Path
sys.path.insert(0, 'tools/python-deps')
import pefile, unicorn as U
from unicorn.x86_const import *
VERSIONS = {
 '181': (0x529f54, 0x52a9f4, 0x52a744, 0x67ec62, 0x676140, 0x675940, 0x67f2bc, 216, 0x4481d7),
 '185': (0x532188, 0x532e6c, 0x532284, 0x6beeb2, 0x6a9900, 0x6a9100, 0x6bfb68, 220, 0x47cd47),
}
def run(version, form, facing, point_count, adapter=False):
 ap, op, gv, bounds, cos, sin, tau, stride, entry = VERSIONS[version]
 name = '1.8.1' if version == '181' else '1.8.5'
 p = pefile.PE('research/extracted/update-' + name + '/Myth II.exe')
 c = U.Uc(U.UC_ARCH_X86, U.UC_MODE_32)
 c.mem_map(0x400000, (p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
 c.mem_write(0x400000, p.get_memory_mapped_image()); c.mem_map(0x1000000, 0x40000)
 def w(a,*v): c.mem_write(a,struct.pack('<'+'I'*len(v),*v))
 for base,fn in [(cos,math.cos),(sin,math.sin)]:
  c.mem_write(base,struct.pack('<1024h',*[round(fn(i*math.tau/1024)*16384) for i in range(1024)]))
 c.mem_write(tau,struct.pack('<f',6.283185307)); w(ap,0x1001000); w(op,0x1003000); w(gv,2181)
 c.mem_write(bounds,struct.pack('<HH',191,223))
 for i in range(4):
  c.mem_write(0x1001000+i*stride+12,struct.pack('<h',i)); w(0x1003000+i*80+8,95000+i*500,78000+i*300)
 c.mem_write(0x1005000,struct.pack('<4H',0,1,2,3))
 for i in range(point_count): w(0x1006000+i*8,95000-i*1000,79000+i*300)
 c.mem_write(0x1007000,struct.pack('<4H',24,form,facing,0))
 sp,stop,center=0x103f000,0x1000000,0x1007100
 if version=='185' and not adapter:
  w(sp,stop,4,0x1005000); c.reg_write(UC_X86_REG_ESP,sp); c.reg_write(UC_X86_REG_ECX,center)
  c.emu_start(0x485d99,stop,count=100000)
  assert c.reg_read(UC_X86_REG_EIP)==stop and c.reg_read(UC_X86_REG_ESP)==sp+4
  assert struct.unpack('<2i',c.mem_read(center,8))==(95750,78450)
 w(sp,stop,0x1007000,0xffffffff,0x1005000,4,0x1006000,0x1008000,0x100a000)
 c.reg_write(UC_X86_REG_ESP,sp); c.reg_write(UC_X86_REG_EAX,point_count); c.reg_write(UC_X86_REG_ECX,center)
 if adapter:
  from verify_185_adapters import load_adapter, saved_registers, assert_registers
  w(sp+32,point_count)
  entry=load_adapter(c,'formation185'); saved_registers(c)
 c.emu_start(entry,stop,count=1000000)
 if adapter:assert_registers(c)
 assert c.reg_read(UC_X86_REG_EIP)==stop and c.reg_read(UC_X86_REG_ESP)==sp+4
 return {'positions':[list(struct.unpack('<8i',c.mem_read(0x1008000+i*0x3f0,32))) for i in range(point_count)],'facings':bytes(c.mem_read(0x100a000,point_count*2)).hex()}
def main():
 rows=[]
 for form in range(10):
  for facing in (0,256,768):
   for points in (1,4):
    a,b=run('181',form,facing,points),run('185',form,facing,points)
    assert a==b,(form,facing,points,a,b)
    rows.append({'formation':form,'facing':facing,'point_count':points,'output':b})
 Path('research/reports/version-185-formation-verification.json').write_text(json.dumps({'result':'pass','cases':rows,'limitations':'Four synthetic units, gameplay version 2181; no collision/pathfinding, assignment optimization or live commands.'},indent=2),encoding='utf8')
 print('PASS: 60 formation cases match Build 434; 471b center helper and stack verified')
 

if __name__ == "__main__": main()


