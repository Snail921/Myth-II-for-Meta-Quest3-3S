"""Capture original height/transfer fade results; only asset lookup/list insertion stubbed."""
from verify_combat_builders import *

def main():
 c=machine()
 def stub(cpu,a,size,user):
  if a not in (0x469816,0x4696b1,0x4be4de):return
  if a!=0x4be4de:cpu.reg_write(UC_X86_REG_EAX,0x100e000 if a==0x469816 else 0x100e100)
  esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0];cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
 c.hook_add(u.UC_HOOK_CODE,stub)
 c.mem_write(0x100d000,bytes(12));c.mem_write(0x100e100,struct.pack('<3h',16,16,16)+bytes(42)+struct.pack('<I',0x100e200));c.mem_write(0x675634,struct.pack('<I',0x1010000))
 cases=[]
 for kind in (0,1):
  c.mem_write(0x100e000,struct.pack('<I2ih',kind,0,0,0)+bytes(18))
  for height in (-1024,-513,-512,-400,-1,0,1,100,512,1024,3072,4000):
   for transfer in (0,1,2,3,5,6):
    for phase in (0,1,16384,32767,32768,65535):
     c.mem_write(0x67563c,struct.pack('<I',0x100f000));c.mem_write(0x100f000,bytes(28))
     run(c,0x4be64d,[0,transfer,phase,height&0xffffffff,65535],{UC_X86_REG_EAX:0,UC_X86_REG_EDX:0,UC_X86_REG_ECX:0x100d000})
     row=c.mem_read(0x100f000,28);strength=struct.unpack_from('<H',row,18)[0];offset=struct.unpack_from('<i',row)[0]
     cases.append(struct.pack('<6i',height,kind,transfer,phase,strength,offset))
 (ROOT/'build/shadow-placement-oracle.bin').write_bytes(struct.pack('<I',len(cases))+b''.join(cases))
 print('Generated',len(cases),'original shadow placement/fade cases')
if __name__=='__main__':main()
