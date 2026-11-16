"""Execute Build 434's actual health-bar ratio instructions in Unicorn."""
import struct
from verify_combat_builders import machine
from unicorn.x86_const import *

for current, maximum in [(256, 256), (128, 256), (194, 561), (0, 256), (10, 0)]:
    c = machine()
    obj = 0x1020000
    c.mem_write(obj + 66, struct.pack('<hh', current, maximum))
    c.reg_write(UC_X86_REG_ECX, obj)
    c.reg_write(UC_X86_REG_EBP, 0x1021000)
    c.reg_write(UC_X86_REG_ESP, 0x1022000)
    c.emu_start(0x4bfaa4, 0x4bfac9, count=100)
    result = c.reg_read(UC_X86_REG_ESI)
    assert result == (current * 65536 // maximum if maximum else 0)
    print(f'current={current} maximum={maximum} original16_16={result}')
print('PASS actual original health bar ratio instructions')
