"""Header-only preview sequences retain indices; malformed dimensions still fail."""
import struct
from prepare_vr_assets import decode
from inspect_surface_effects import inspect

def fixture(views=0, frames=0, size=64):
    b=bytearray(512)
    def word(at,v):struct.pack_into('>I',b,at,v)
    word(248,256);word(68,0);word(128,1);word(132,32)
    b[288:293]=b'empty';word(288+64,160);word(288+68,size)
    struct.pack_into('>2h',b,416+8,views,frames)
    return b

assert decode(fixture())[2]==[(0,0.0,[])]
assert inspect(fixture())['sequences'][0]['maps']==[]
for args in [(0,1,64),(1,0,64),(-1,0,64),(0,0,65),(65,1,64)]:
    for reader in (decode,inspect):
        try:reader(fixture(*args))
        except ValueError:pass
        else:raise AssertionError((reader.__name__,args))
print('PASS empty preview sequence preserves index; invalid dimensions rejected')
