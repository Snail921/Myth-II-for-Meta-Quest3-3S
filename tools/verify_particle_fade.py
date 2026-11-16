"""Original particle intensity curve 0x430a80, assets stubbed, arithmetic unmodified."""
from verify_combat_builders import *
def main():
 c=machine();c.mem_write(0x52966c,struct.pack('<I',0x100d000));c.mem_write(0x5297cc,struct.pack('<I',0x100e000));c.mem_write(0x100e148,struct.pack('<I',0x100e200))
 def stub(cpu,a,size,user):
  if a!=0x4699ba:return
  esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0];cpu.reg_write(UC_X86_REG_EAX,0x100e100);cpu.reg_write(UC_X86_REG_EDX,0x100d000);cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
 c.hook_add(u.UC_HOOK_CODE,stub);cases=[]
 for mode in range(5):
  for age in (0,1,10,29,30,50):
   for frame in (0,1,7,8):
    for ft in (0,2):
     life,tpf,frames=30,3,8;c.mem_write(0x100e02c,struct.pack('<h',mode));c.mem_write(0x100d050,struct.pack('<2i',age,life));c.mem_write(0x100d049,bytes([frame,ft,tpf]));c.mem_write(0x100e20a,struct.pack('<h',frames))
     run(c,0x430a80,[],{UC_X86_REG_ECX:0,UC_X86_REG_EDX:0});value=c.reg_read(UC_X86_REG_EAX)&0xffffffff
     cases.append(struct.pack('<8I',mode,age,life,frame,ft,tpf,frames,value))
 (ROOT/'build/particle-fade-oracle.bin').write_bytes(struct.pack('<I',len(cases))+b''.join(cases));print('Generated',len(cases),'original particle fade cases')
if __name__=='__main__':main()
