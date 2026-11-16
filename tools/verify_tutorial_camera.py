"""Run compiled bridge dispatcher and original camera SNIF; stub only scenario callback targets."""
import struct,json
from verify_combat_builders import machine,run,ROOT,STOP,pefile
from unicorn.x86_const import *
c=machine();dll=pefile.PE(str(ROOT/'build/input-x86/input-test.dll'));base=dll.OPTIONAL_HEADER.ImageBase
c.mem_map(base,(dll.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);c.mem_write(base,dll.get_memory_mapped_image())
entry=base+next(e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'MythTestExecute')
mesh=STOP+0xb000
c.mem_write(0x67ec00,struct.pack('<H',3));c.mem_write(0x529bd4,struct.pack('<I',mesh));c.mem_write(mesh,struct.pack('<I4xHH',0x3030746d,3,3));c.mem_write(0x52a9f4,struct.pack('<I',STOP+0xc000))
# Real SNIF calls these scenario activators. Count success/failure instead of needing a live scenario graph.
success=STOP+0x27000;failure=success+4
c.mem_write(0x49c145,b'\xb8'+struct.pack('<I',STOP+0x26000)+b'\xc3')
c.mem_write(0x49b9b1,b'\xff\x05'+struct.pack('<I',success)+b'\xc3')
c.mem_write(0x49ba6c,b'\xff\x05'+struct.pack('<I',failure)+b'\xc3')
camera=bytes(c.mem_read(0x54e43c,40))
def send(mask,flags=None):
 if flags is None:flags=123<<16|struct.unpack("<H",c.mem_read(0x561872,2))[0]
 addr=STOP+0x20000;c.mem_write(addr,struct.pack('<8I',0x31444d43,1,21,mask,flags,0,0,0));run(c,entry,[addr],{});return c.reg_read(UC_X86_REG_EAX)
def stage(case):
 c.mem_write(0x561868,struct.pack('<I',8));c.mem_write(0x561870,struct.pack('<HHI',123,case,999999));c.mem_write(success,bytes(8))
for case in range(51,56):
 for bit in range(10):
  stage(case);assert send(1<<bit)==1
  good=bit//2==case-51
  assert struct.unpack('<I',c.mem_read(success,4))[0]==int(good),(case,bit)
  assert struct.unpack('<I',c.mem_read(failure,4))[0]==0
  assert (struct.unpack('<I',c.mem_read(0x561868,4))[0]&4)!=0 if good else c.mem_read(0x561868,4)==struct.pack('<I',8)
  assert bytes(c.mem_read(0x54e43c,40))==camera
# Non-tutorial level, incorrect dimensions, paused/loading/end, other lesson: no advancement.
for tag,w,h,flags,lesson in [(0x6c312020,3,3,0,51),(0x3030746d,7,6,0,51),(0x3030746d,3,3,2,51),(0x3030746d,3,3,16,51),(0x3030746d,3,3,32,51),(0x3030746d,3,3,0,100)]:
 c.mem_write(mesh,struct.pack('<I4xHH',tag,w,h));c.mem_write(0x67ec10,struct.pack('<I',flags));stage(lesson);assert send(1023)==1;assert c.mem_read(success,8)==bytes(8)
stage(51);assert send(1,124<<16|51)==1;assert c.mem_read(success,4)==bytes(4)
for mask,flags in [(0,0),(1024,0)]:assert send(mask,flags)==16
report={'result':'pass','direction_cases':50,'original_sniffer':'0x46a80a','bit_reader':'0x48fbc5','active_case':'0x561872','scenario_callback_stubs':['0x49c145','0x49b9b1','0x49ba6c'],'camera_unchanged':True,'other_maps_and_non_camera_lessons_unchanged':True,'device_validation':'pending'}
(ROOT/'research/reports/tutorial-camera-verification.json').write_text(json.dumps(report,indent=2))
print('PASS 50 camera-event cases; no camera mutation; map/stage/pause/packet guards')
