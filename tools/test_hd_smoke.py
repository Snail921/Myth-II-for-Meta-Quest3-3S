import runpy,struct,json
from pathlib import Path
g=runpy.run_path('tools/test_hd_hues.py');globals().update(g)
data=Path('build/hd-test/smoke.bin').read_bytes();neutral=bytes.fromhex(json.loads(Path('build/hd-test/live-hues.json').read_text())['261']);assert struct.unpack_from('<I',neutral,80)[0]==1
controls=bytearray(64);flags=struct.unpack_from('<H',neutral,6)[0];controls[:8]=bytes([0 if flags&4 else 1,255 if flags&1 else 0,128,neutral[1]])+neutral[2:6]
path=root/'smoke-colored.bin';path.write_bytes(b'HUEIMG01'+controls+data);call('push',str(path),'/data/local/tmp/hd-image.bin');call('shell','/data/local/tmp/myth-hd-test /data/local/tmp/hd-image.bin /data/local/tmp/hd-image.out');call('pull','/data/local/tmp/hd-image.out',str(root/'smoke-colored.rgba'));actual=(root/'smoke-colored.rgba').read_bytes()
w,h,_,flags=struct.unpack_from('>4H',data);skip=52+(0 if flags&32 else 4*(h-1));header=bytearray(52);struct.pack_into('<2h',header,0,w,h);struct.pack_into('<I',header,44,pixels);u.mem_write(bitmap,bytes(header));u.mem_write(table,bytes(64)+neutral+bytes(8));u.mem_write(pixels,data[skip:]);u.mem_write(sp,struct.pack('<3I',stop,bitmap,output));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EDX,table);u.emu_start(0x4500a0,stop,count=10000000);native=bytes(u.mem_read(output,w*h*4));converted=bytearray()
for i in range(0,len(actual),4):
 r,g,b,a=actual[i:i+4];converted+=bytes((b*(a+1)>>8,g*(a+1)>>8,r*(a+1)>>8,a))
assert converted==native
print('PASS actual smoke with captured black-smoke palette: byte-exact against original 1.8.5')
