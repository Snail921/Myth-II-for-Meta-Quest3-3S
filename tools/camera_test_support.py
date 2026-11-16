import struct
from verify_combat_builders import STOP

def win32_stubs(c,dll):
 stubs={b'VirtualProtect':bytes.fromhex('8b442410c70020000000b801000000c21000'),b'FlushInstructionCache':bytes.fromhex('b801000000c20c00'),b'CreateFileA':bytes.fromhex('b801000000c21c00'),b'WriteFile':bytes.fromhex('b801000000c21400'),b'CloseHandle':bytes.fromhex('b801000000c20400')}
 for desc in dll.DIRECTORY_ENTRY_IMPORT:
  for imp in desc.imports:
   if imp.name in stubs:
    addr=STOP+0x28000+list(stubs).index(imp.name)*128;c.mem_write(addr,stubs[imp.name]);c.mem_write(imp.address,struct.pack('<I',addr))
