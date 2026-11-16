"""Verify the original 471b archive-priority permutation builder in Unicorn."""
import sys,struct,random,json
from pathlib import Path
sys.path.insert(0,'tools/python-deps')
import pefile,unicorn as U
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');image=p.get_memory_mapped_image();u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
u.mem_map(0x400000,(len(image)+4095)&~4095);u.mem_write(0x400000,image)
scratch=0x7f000000;u.mem_map(scratch,65536);pool=scratch;records=scratch+256;requested=scratch+16000;a=scratch+17000;b=scratch+18000;sp=scratch+60000;stop=scratch+65000
u.mem_write(0x6a1f50,struct.pack('<I',pool));r=random.Random(185)
for case in range(128):
 n=1+case%32;order=list(range(n));r.shuffle(order)
 u.mem_write(pool+18,struct.pack('<H',n));u.mem_write(pool+24,struct.pack('<I',records))
 for i in range(n):u.mem_write(records+i*408,struct.pack('<HHI',1,0,0x900000+i*256))
 u.mem_write(requested,struct.pack('<'+'I'*n,*[0x900000+i*256 for i in order]));u.mem_write(sp,struct.pack('<5I',stop,requested,n,a,b));u.reg_write(UC_X86_REG_ESP,sp)
 u.emu_start(0x4d5d08,stop,count=50000);assert u.reg_read(UC_X86_REG_EIP)==stop
 assert list(struct.unpack('<'+'H'*n,u.mem_read(a,n*2)))==[order.index(i) for i in range(n)]
 assert list(struct.unpack('<'+'H'*n,u.mem_read(b,n*2)))==order
print('PASS 128 original archive-order permutations, 1..32 archives')
Path('research/reports/active-plugin-order-emulation.json').write_text(json.dumps({'engine':'1.8.5 Build471b','routine':'0x4d5d08','pool_pointer':'0x6a1f50','stride':408,'cases':128,'result':'pass','scope':'original order permutation generation; live capture pending'},indent=2))

# Original tag winner selection: larger archive index wins; local (-1) wins all.
tagpool=scratch+20000;tagrecords=scratch+21000;group=scratch+25000;indices=scratch+25100
u.mem_write(0x6a1e00,struct.pack('<I',tagpool));u.mem_write(tagpool+24,struct.pack('<I',tagrecords));u.mem_write(0x6a1f0a,b'\0')
def group_accessor(uc,address,size,data):
 if address==0x4d5744:
  esp=uc.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',uc.mem_read(esp,4))[0]
  uc.reg_write(UC_X86_REG_EAX,group);uc.reg_write(UC_X86_REG_ESP,esp+4);uc.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(U.UC_HOOK_CODE,group_accessor)
for case in range(96):
 n=2+case%15;slots=list(range(n));r.shuffle(slots)
 if case%2:slots[0]=-1
 u.mem_write(group,struct.pack('<HHI',n,n,indices));u.mem_write(indices,struct.pack('<'+'H'*n,*range(n)))
 for i,slot in enumerate(slots):
  row=bytearray(68);struct.pack_into('<H',row,0,1);struct.pack_into('<II',row,36,0x2e323536,0x74657374);row[59]=slot&255;u.mem_write(tagrecords+i*68,bytes(row))
 u.mem_write(sp,struct.pack('<5I',stop,0x2e323536,0x74657374,0xffffffff,0xffffffff));u.reg_write(UC_X86_REG_ESP,sp)
 u.emu_start(0x4d5932,stop,count=100000);assert u.reg_read(UC_X86_REG_EIP)==stop
 winners=[i for i in range(n) if not u.mem_read(tagrecords+i*68+2,1)[0]&1]
 assert winners==[slots.index(-1) if -1 in slots else slots.index(max(slots))],(slots,winners)
print('PASS 96 original tag-winner cases: later archives override earlier; local tags override archives')
