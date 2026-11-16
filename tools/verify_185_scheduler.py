"""Verify Build 471b scheduler in a synthetic local-game state (no live writes)."""
import runpy,struct,json,contextlib,io
from pathlib import Path
# Reuse the verified Film-based builder fixture and its isolated emulator.
with contextlib.redirect_stdout(io.StringIO()): s=runpy.run_path('tools/verify_185_movement.py')
u,w,sp,stop=s['u'],s['w'],s['sp'],s['stop']
from unicorn.x86_const import *
ready,ready_bytes=0x1002400,0x1004000
w(0x540a4c,ready);w(ready,0,0,0,0,0,0,2048,ready_bytes);u.mem_write(ready_bytes,b'\xa5'*2048)
u.mem_write(0x6bee46,struct.pack('<h',2));u.mem_write(0x68f6ba,b'\0\0');u.mem_write(0x670fc0,b'\0\0')
tick,player=s['command']['tick'],s['command']['player'];w(0x6bee78,tick);w(0x5332dc,player)
w(sp,stop);u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EDI,1)
u.emu_start(0x48eb22,stop,count=200000)
assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==sp+4
state=struct.unpack('<8I',u.mem_read(ready,32));data=bytes(u.mem_read(ready_bytes,state[2]))
assert data[:8]==struct.pack('<hbbI',s['command']['size'],3,player,tick)
assert data[8:]==s['generated'][8:]
pending=struct.unpack('<8I',u.mem_read(s['queue'],32));assert pending[1]==pending[2]
assert struct.unpack('<I',u.mem_read(0x6bee78,4))[0]==tick+1
assert bytes(u.mem_read(ready_bytes+len(data),16))==b'\xa5'*16
Path('research/reports/version-185-scheduler-verification.json').write_text(json.dumps({'result':'pass','scheduler':'0x48eb22','abi':'DI = tick increment; no stack arguments','ready_command_hex':data.hex(),'pending_drained':True,'tick_incremented':True,'limitations':'Synthetic game mode 2, tutorial filter disabled, recorder inactive; no live command execution, recording, networking or pathfinding.'},indent=2),encoding='utf8')
print('PASS: 471b scheduler assigns Film player/tick, transfers movement to ready queue and increments tick')
