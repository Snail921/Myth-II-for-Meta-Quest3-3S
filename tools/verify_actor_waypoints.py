"""Verify the original 1.8.1 per-unit movement storage in an isolated x86 emulator."""
import struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile,unicorn as u
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
pe=pefile.PE(str(ROOT/'research/extracted/update-1.8.1/Myth II.exe'))
for n in range(1,5):
 c=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
 c.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(0x400000,pe.get_memory_mapped_image());c.mem_map(0x1000000,0x20000)
 actors,objects,points,stack,stop=0x1001000,0x1002000,0x1003000,0x101f000,0x1000000
 def w(a,v):c.mem_write(a,struct.pack('<I',v))
 w(0x529f54,actors);w(0x52a9f4,objects);w(0x52a744,2181);w(0x529284,0x1004000)
 expected=b''.join(struct.pack('<ii',95000+i*500,78000-i*750) for i in range(n))
 c.mem_write(points,expected);c.mem_write(actors+0x8b,b'\xff')
 c.mem_write(stack,struct.pack('<6I',stop,0,n,points,8,12345));c.reg_write(UC_X86_REG_ESP,stack)
 c.emu_start(0x448654,stop,count=100000)
 assert c.reg_read(UC_X86_REG_EIP)==stop
 assert bytes(c.mem_read(actors+0x22,2))==bytes([n,0])
 assert bytes(c.mem_read(actors+0x28,n*8))==expected
 assert struct.unpack('<H',c.mem_read(actors+0x26,2))[0]==12345
print('PASS original 0x448654: actor count +0x22, index +0x23, points +0x28, facing +0x26 (1..4 points)')
