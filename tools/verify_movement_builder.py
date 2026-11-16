"""Run the original movement builder with a minimal synthetic actor table/queue.

No live game access. Synthetic state verifies serialization, not gameplay behavior.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile
import unicorn as u
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from inspect_film import parse


def main():
    binary=(ROOT/'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    film=(ROOT/'research/fixtures/films/capture-01.film').read_bytes()
    parsed=parse(film)
    cmd=next(c for c in parsed['commands'] if c['verb']==3)
    move=cmd['movement_candidate']
    assert len(move['unit_ids'])==1 and len(move['waypoints'])==1
    pe=pefile.PE(data=binary)
    uc=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
    uc.mem_map(pe.OPTIONAL_HEADER.ImageBase,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
    uc.mem_write(pe.OPTIONAL_HEADER.ImageBase,pe.get_memory_mapped_image())
    uc.mem_map(0x1000000,0x20000)
    stack,stop=0x101f000,0x1000000
    actor_table,indices,opts,points,queue,queue_bytes=0x1001000,0x1002000,0x1002100,0x1002200,0x1002300,0x1003000
    def write32(addr,value):
        uc.mem_write(addr,struct.pack('<I',value))
    # Actor slot 2 is deliberately different from the Film's stable ID 10011.
    slot=2
    write32(0x529f54,actor_table)
    uc.mem_write(actor_table+slot*0xd8,struct.pack('<h',move['unit_ids'][0]))
    uc.mem_write(indices,struct.pack('<h',slot))
    uc.mem_write(opts,struct.pack('<hHH2x',move['flags'],move['formation_index'],move['final_facing_raw']))
    point=move['waypoints'][0]
    uc.mem_write(points,struct.pack('<ii',point['raw_x'],point['raw_y']))
    write32(0x52a744,parsed['game_parameters_candidate']['gameplay_version'])
    write32(0x52f4e4,queue)
    uc.mem_write(queue,struct.pack('<7I',0,0,0,0,0,2048,queue_bytes))
    uc.mem_write(queue_bytes,b'\xa5'*2048)
    # Reviewed calling convention: ECX=slot list, stack=(mode,opts,count,point count,points).
    uc.mem_write(stack,struct.pack('<6I',stop,0xffffffff,opts,1,1,points))
    uc.reg_write(UC_X86_REG_ESP,stack)
    uc.reg_write(UC_X86_REG_ECX,indices)
    uc.emu_start(0x446764,stop,count=100000)
    assert uc.reg_read(UC_X86_REG_EIP)==stop,'Builder did not return'
    assert uc.reg_read(UC_X86_REG_ESP)==stack+4,'Unexpected caller stack'
    queue_state=struct.unpack('<7I',uc.mem_read(queue,28))
    generated=bytes(uc.mem_read(queue_bytes,queue_state[2]))
    assert len(generated)==cmd['size'],(len(generated),cmd['size'])
    assert struct.unpack('<hbbI',generated[:8])==(cmd['size'],3,-1,0)
    assert generated[8:]==bytes.fromhex(cmd['payload_hex']),'Original generated payload differs from Film'
    assert bytes(uc.mem_read(queue_bytes+len(generated),16))==b'\xa5'*16
    # Execute the original scheduling path with a synthetic mode-2 state.
    # Recorder inactive and compatibility filter mode 0 avoid OS I/O; neither routine is patched.
    ready_queue,ready_bytes,recorder=0x1002400,0x1004000,0x1005000
    write32(0x52f4e8,ready_queue)
    uc.mem_write(ready_queue,struct.pack('<7I',0,0,0,0,0,2048,ready_bytes))
    uc.mem_write(ready_bytes,b'\xa5'*2048)
    write32(0x553eec,recorder)
    uc.mem_write(recorder,b'\x00'*0x840)
    uc.mem_write(0x67ec06,struct.pack('<h',2))
    uc.mem_write(0x561872,struct.pack('<h',0))
    write32(0x67ec38,cmd['tick'])
    write32(0x52ae68,cmd['player'])
    uc.mem_write(stack,struct.pack('<2I',stop,1))
    uc.reg_write(UC_X86_REG_ESP,stack)
    uc.emu_start(0x44c631,stop,count=100000)
    assert uc.reg_read(UC_X86_REG_EIP)==stop,'Scheduler did not return'
    assert uc.reg_read(UC_X86_REG_ESP)==stack+4
    ready_state=struct.unpack('<7I',uc.mem_read(ready_queue,28))
    assert ready_state[2]==cmd['size']
    pending_after=struct.unpack('<7I',uc.mem_read(queue,28))
    assert pending_after[1]==pending_after[2], 'Pending queue was not drained'
    assert bytes(uc.mem_read(ready_bytes+cmd['size'],16))==b'\xa5'*16
    stamped=bytes(uc.mem_read(ready_bytes,ready_state[2]))
    assert struct.unpack('<hbbI',stamped[:8])==(cmd['size'],3,cmd['player'],cmd['tick'])
    assert stamped[8:]==generated[8:]
    next_tick=struct.unpack('<I',uc.mem_read(0x67ec38,4))[0]
    assert next_tick==cmd['tick']+1
    uc.mem_write(stack,struct.pack('<I',stop))
    uc.reg_write(UC_X86_REG_ESP,stack)
    uc.reg_write(UC_X86_REG_ECX,ready_bytes)
    uc.reg_write(UC_X86_REG_EAX,0)
    uc.emu_start(0x408b6a,stop,count=10000)
    assert uc.reg_read(UC_X86_REG_EIP)==stop
    encoded=bytes(uc.mem_read(ready_bytes,len(generated)))
    original=film[cmd['file_offset']:cmd['file_offset']+cmd['size']]
    assert encoded==original
    report=dict(result='pass',binary_sha256=hashlib.sha256(binary).hexdigest(),
                film_sha256=hashlib.sha256(film).hexdigest(),builder_va='0x00446764',
                synthetic_actor_slot=slot,stable_id=move['unit_ids'][0],
                generated_pre_schedule_hex=generated.hex(),film_command_hex=original.hex(),
                payload_matches=True,full_command_matches_after_original_scheduler=True,
                scheduler_va='0x0044c631',scheduler_mode=2,filter_mode=0,
                recorder_active=False,scheduled_header_hex=stamped[:8].hex(),next_tick=next_tick,
                queue_state_after_builder=queue_state,pending_queue_after_scheduler=pending_after,
                ready_queue_after_scheduler=ready_state,
                limitations='Synthetic actor table, empty queues, mode-2 state with filter 0 and inactive recorder. Tick/player globals initialized from Film. No mouse input, pathfinding or unit motion executed; scheduler in other modes untested.')
    (ROOT/'research/reports/movement-builder-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('PASS: original builder converts slot 2 to ID 10011 and emits the exact Film movement payload.')
    print('PASS: original scheduler assigns player/tick and moves the command to the ready queue.')
    print('PASS: after original header conversion, all 30 command bytes match the Film.')


if __name__=='__main__':
    main()
