"""Execute the unmodified 471b tag lookup against synthetic registry memory."""
import sys,struct,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'python-deps'))
import pefile,unicorn as U
from unicorn.x86_const import *
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');image=p.get_memory_mapped_image()
u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32);u.mem_map(0x400000,(len(image)+4095)&~4095);u.mem_write(0x400000,image)
assert image[0x133dd0:0x133dda]==b'collection'
assert struct.unpack_from('<I',image,0x133df0)[0]==int.from_bytes(b'.256','big')
scratch=0x7f000000;u.mem_map(scratch,65536);registry=scratch;ids=scratch+4096;sp=scratch+60000;stop=scratch+65000
tags=[b'fire',b'11cf',b'mafi',b'chfi',b'mapl',b'chmp'];values=[int.from_bytes(t,'big') for t in tags]
u.mem_write(registry+0x44,struct.pack('<H',len(tags)));u.mem_write(registry+0x54,struct.pack('<I',ids));u.mem_write(ids,struct.pack('<6I',*values));writes=[]
u.hook_add(U.UC_HOOK_MEM_WRITE,lambda uc,access,a,n,v,data:writes.append(a) if not scratch<=a<a+n<=scratch+65536 else None)
for expected,tag in enumerate(tags+[b'none']):
 u.mem_write(sp,struct.pack('<II',stop,int.from_bytes(tag,'big')));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EDX,registry)
 u.emu_start(0x438932,stop,count=1000)
 assert u.reg_read(UC_X86_REG_EIP)==stop
 assert u.reg_read(UC_X86_REG_EAX)==(expected if expected<6 else 0xffffffff)
assert not writes
report={'result':'pass','engine':'1.8.5-471b','registry':'0x533dd0','count':'0x533e14 (u16)','ids_pointer':'0x533e24','lookup':'0x438932','cases':[t.decode() for t in tags]+['absent ID'],'writes_outside_emulator_scratch':writes,'live_game_validation':False}
Path('research/reports/collection-tag-lookup.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('PASS original 471b lookup separates all 3 collision pairs and rejects unknown ID; no game-memory writes')
