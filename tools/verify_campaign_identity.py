"""Compare campaign fingerprints with original Build 434's native header decoder."""
import hashlib,struct
from campaign_levels import campaign,identity,IDENTITY_OFFSETS
from level_assets import inventory,raw_tag,parse_mesh,schemas
from verify_combat_builders import machine,run,STOP
from unicorn.x86_const import *

tags=inventory();levels=campaign(tags);seen=set()
for mapid,name in levels:
 raw=raw_tag(tags,'mesh',mapid);c=machine();at=STOP+0x20000;c.mem_write(at,raw[:1024])
 run(c,0x481677,[0x51a1d8,1024,0x529ab0],{UC_X86_REG_EAX:1,UC_X86_REG_ECX:at,UC_X86_REG_EDX:0})
 native=bytes(c.mem_read(at,1024));actual=hashlib.sha256(b''.join(native[o:o+4] for o in IDENTITY_OFFSETS)).hexdigest()
 assert actual==identity(raw),(mapid,actual,identity(raw))
 assert actual not in seen;seen.add(actual)
 changed=bytearray(native);changed[52]^=1
 assert hashlib.sha256(b''.join(changed[o:o+4] for o in IDENTITY_OFFSETS)).hexdigest()!=actual
 print('PASS',ascii((mapid,name)))
assert len(levels)==27
print('PASS 27 unique official single-player identities, original endian conversion, changed-header rejection')
