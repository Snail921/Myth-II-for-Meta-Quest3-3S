"""Execute compiled camera IPC dispatcher and original setter/listener without stubs."""
import struct,json
from verify_combat_builders import machine,ROOT,run,pefile
from unicorn.x86_const import *
c=machine();dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'));base=dll.OPTIONAL_HEADER.ImageBase
c.mem_map(base,(dll.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(base,dll.get_memory_mapped_image())
from camera_test_support import win32_stubs
win32_stubs(c,dll)
entry=base+next(e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'MythTestExecute')
c.mem_write(0x67ec00,struct.pack('<H',3));c.mem_write(0x529bd4,struct.pack('<I',0x100b000));c.mem_write(0x100b008,struct.pack('<HH',7,6));c.mem_write(0x52a9f4,struct.pack('<I',0x100c000))
c.mem_write(0x54e45c,struct.pack('<f',-.5));c.mem_write(0x54e460,struct.pack('<f',1.))
def send(x,y,z,yaw=0):
 c.mem_write(0x1020000,struct.pack('<5I2iI',0x31444d43,1,12,yaw,0,x,y,z&0xffffffff));run(c,entry,[0x1020000],{});return c.reg_read(UC_X86_REG_EAX)
for x,y,z,yaw in [(95846,80025,1536,0),(64000,51200,2560,16384),(10240,15360,-512,32768)]:
 assert send(x,y,z,yaw)==1
 assert struct.unpack('<3f',c.mem_read(0x54e43c,12))==(x/512,y/512,z/512)
 assert abs(struct.unpack('<f',c.mem_read(0x54e458,4))[0]-yaw*6.283185307179586/65536)<1e-6
 assert struct.unpack('<f',c.mem_read(0x54e45c,4))[0]==-.5
 assert struct.unpack('<f',c.mem_read(0x54e460,4))[0]==1.
 run(c,0x44ea02,[],{});p=c.reg_read(UC_X86_REG_EAX)
 listener=struct.unpack('<3i',c.mem_read(p,12));assert listener==(x,y,z),(listener,(x,y,z))
prior=bytes(c.mem_read(0x54e43c,36))
for flags in (2,16,32):
 c.mem_write(0x67ec10,struct.pack('<I',flags));assert send(1,2,3)==1;assert bytes(c.mem_read(0x54e43c,36))==prior
c.mem_write(0x67ec10,bytes(4));assert send(-1,2,3)==16;assert send(1,2,70000)==16
c.mem_write(0x67ec00,struct.pack('<H',14));assert send(1,2,3)==20
print('PASS compiled camera IPC, original setter/listener, retained pitch/zoom, pause/end/loading and bounds (Win32 API stubs only)')
(ROOT/'research/reports/camera-sync-verification.json').write_text(json.dumps(dict(result='pass',original_functions_stubbed=False),indent=2))
