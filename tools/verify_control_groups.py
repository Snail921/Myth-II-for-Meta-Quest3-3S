"""Original control-group registration/recall with compiled VR dispatcher.
Selection and UI/voice endpoints are stubbed; original group store and event builder execute.
"""
from verify_group_selection import *
from verify_combat_builders import QUEUE,BYTES

def endpoints(cpu,address,size,user):
 if address not in (0x4656c8,0x465aec,0x4200d3,0x481ad9,0x4813a4,0x466be9):return
 esp=cpu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',cpu.mem_read(esp,4))[0]
 if address==0x466be9:cpu.reg_write(UC_X86_REG_EAX,int(cpu.reg_read(UC_X86_REG_EAX)<126))
 else:cpu.reg_write(UC_X86_REG_EAX,0x5179bf)
 cpu.reg_write(UC_X86_REG_ESP,esp+4);cpu.reg_write(UC_X86_REG_EIP,ret)
c.hook_add(u.UC_HOOK_CODE,endpoints)
c.mem_write(0x657bf0,struct.pack('<I',0x1021000))
def sendgroup(op,slot,add=0):return send(op,struct.pack('<5I',slot,add,0,0,0))
def setsel(ids):
 c.mem_write(0x553fd8,struct.pack('<HH',len(ids),0))
 c.mem_write(0x553fdc,struct.pack('<'+'H'*len(ids),*ids))
def group(slot):
 n=struct.unpack('<H',c.mem_read(0x554410+256*slot,2))[0]
 return list(struct.unpack('<'+'H'*n,c.mem_read(0x554414+256*slot,n*2)))
for slot in range(10):
 c.mem_write(QUEUE,struct.pack('<7I',0,0,0,0,0,2048,BYTES));c.mem_write(BYTES,b'\xa5'*2048)
 setsel([2,3]);assert sendgroup(19,slot)==1;assert group(slot)==[2,3];assert selected()==[2,3]
 setsel([4]);assert sendgroup(20,slot)==1;assert selected()==[2,3]
 setsel([3,4]);assert sendgroup(20,slot,1)==1;assert selected()==[3,4,2]
 setsel([]);assert sendgroup(19,slot)==1;assert group(slot)==[]
 setsel([4]);assert sendgroup(20,slot,1)==1;assert selected()==[4]
 assert sendgroup(20,slot)==1;assert selected()==[]
assert sendgroup(19,10)==16 and sendgroup(20,10)==16 and sendgroup(19,0,1)==16
# Invalid stale actor slot is rejected before original selector dereferences it.
c.mem_write(0x554410,struct.pack('<HHH',1,0,400));setsel([4]);assert sendgroup(20,0)==13 and selected()==[4]
print('PASS original group store/event + recall: ten slots, replacement, additive dedup, empty clearing and bounds')
