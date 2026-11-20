"""Emulate compiled command adapter plus actual 471b annotation routines; no live writes."""
from pathlib import Path
import sys,subprocess,struct,json
sys.path.insert(0,str(Path(__file__).parent/'python-deps'))
import pefile,unicorn as U
from unicorn.x86_const import *

w=Path('build/chat-test');w.mkdir(exist_ok=True)
w.joinpath('test.c').write_text('#include "win_min.h"\nstruct Command {U magic,seq,op,id,flags;I x,y;U reserved;};\n#include "chat.h"\nU test(const struct Command*c){return chat_command(c);}\nvoid receive(I p,const char*t,I m){chat_received(p,t,m);}\nvoid* events(void){return &chatEvents;}',encoding='utf8')
bin=Path('tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin')
subprocess.run([str(bin/'clang.exe'),'--target=i686-pc-windows-msvc','-O2','-ffreestanding','-fno-stack-protector','-I','src/input','-I','src/input/engine185','-c',str(w/'test.c'),'-o',str(w/'test.obj')],check=True)
subprocess.run([str(bin/'ld.lld.exe'),'-flavor','link','/dll','/noentry','/machine:x86','/nodefaultlib','/safeseh:no','/export:test','/export:receive','/export:events','/out:'+str(w/'test.dll'),str(w/'test.obj')],check=True)
p=pefile.PE('research/extracted/update-1.8.5/Myth II.exe');d=pefile.PE(str(w/'test.dll'));u=U.Uc(U.UC_ARCH_X86,U.UC_MODE_32)
for pe in (p,d):
 u.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);u.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
u.mem_map(0x1000000,65536)
def put(a,v):u.mem_write(a,struct.pack('<I',v))
def get(a):return struct.unpack('<I',u.mem_read(a,4))[0]
exports={s.name.decode():d.OPTIONAL_HEADER.ImageBase+s.address for s in d.DIRECTORY_ENTRY_EXPORT.symbols}
packets=[];received=[]
def hook(c,a,n,data):
 if a not in (0x47b94a,0x435a84):return
 sp=c.reg_read(UC_X86_REG_ESP)
 if a==0x47b94a:
  flag,text=struct.unpack('<II',c.mem_read(sp+4,8));packets.append((get(flag)&65535,bytes(c.mem_read(text,192)).split(bytes([0]))[0]))
 else:received.append(struct.unpack('<III',c.mem_read(sp+4,12)))
 c.reg_write(UC_X86_REG_EIP,get(sp));c.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(U.UC_HOOK_CODE,hook)
def invoke(name,args):
 sp=0x100e000;stop=0x1000000;u.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),stop,*args))
 u.reg_write(UC_X86_REG_ESP,sp)
 for reg in [UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0x12345678)
 u.emu_start(exports[name],stop,count=100000)
 assert u.reg_read(UC_X86_REG_ESP)==sp+4
 for reg in [UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:assert u.reg_read(reg)==0x12345678
 return u.reg_read(UC_X86_REG_EAX)
def call(op,id=0,flags=0,payload=None):
 data=struct.pack('<8I',0,1,op,id,flags,0,0,0)
 if payload is not None:data=data[:12]+payload.ljust(20,bytes([0]))
 u.mem_write(0x1002000,data);return invoke('test',[0x1002000])
put(0x6bee46,1);put(0x5332dc,0);put(0x533296,2);put(0x540a58,0xffff);put(0x531618,0x1003000)
for mode in (0,1):
 text=b'x'*191
 assert call(26,mode,len(text))==1
 for i in range(0,len(text),20):assert call(27,payload=text[i:i+20])==1
 assert call(28)==1 and packets[-1]==(mode,text)
 assert call(28)==19 # no duplicate commit
assert call(26,0,192)==16 and call(26,0,0)==16
assert call(26,0,2)==1 and call(28)==16 # incomplete transaction
assert call(26,0,1)==1 and call(27,payload=b'\n')==16
assert call(26,0,1)==1
put(0x531618,0x1004000);assert call(27,payload=b'x')==19
put(0x6bee46,2);assert call(26,0,1)==20
put(0x6bee46,1);put(0x540a58,0);assert call(26,0,1)==20
# Complete messages, repeats, bounded overflow; original display still called.
put(0x53329c,0x1005000);u.mem_write(0x1005000+24,b'Player'+bytes([0]))
u.mem_write(0x1005000+88,bytes([0,50,0,100,0,150]))
u.mem_write(0x1006000,b'repeated message'+bytes([0]))
for i in range(20):invoke('receive',[0,0x1006000,i%2])
addr=invoke('events',[]);assert get(addr+8)==20 and get(addr+12)%2==0
for seq in range(5,21):
 row=addr+16+(seq-1)%16*240;assert get(row)==seq
 assert bytes(u.mem_read(row+12,3))==bytes([50,100,150])
 assert bytes(u.mem_read(row+48,17))==b'repeated message'+bytes([0])
assert len(received)==20
print('PASS chat ABI, modes, 191-byte limit, transaction guards, replay/disconnect rejection, repeated events, ring overflow, original-display forwarding')

# Exercise actual original recipient filtering and SE branch with decoder stubbed.
sounds=[]
def decoder(c,a,n,data):
 if a not in (0x434439,0x4344bd,0x415968):return
 sp=c.reg_read(UC_X86_REG_ESP)
 if a==0x434439:c.reg_write(UC_X86_REG_EAX,0x1007000)
 elif a==0x4344bd:c.reg_write(UC_X86_REG_EAX,0x1006000)
 else:sounds.append(c.reg_read(UC_X86_REG_ECX))
 c.reg_write(UC_X86_REG_EIP,get(sp));c.reg_write(UC_X86_REG_ESP,sp+4)
u.hook_add(U.UC_HOOK_CODE,decoder)
exports['original_receive']=0x47deab
put(0x5332e0,0);put(0x6bee46,1);put(0x6bee54,0)
u.mem_write(0x1007100,bytes(8))
for flag,team,expected in [(1,0,True),(1,1,False),(0,0,True),(0,1,True)]:
 put(0x1007000,flag);u.mem_write(0x100500a,bytes([team]));before=len(received);sound_before=len(sounds)
 u.reg_write(UC_X86_REG_EAX,0x1007100);invoke('original_receive',[])
 assert (len(received)>before)==expected and (len(sounds)>sound_before)==expected
 if expected:assert sounds[-1]==0x15+flag
print('PASS original Whisper team isolation, Yell broadcast acceptance, original SE selection')
