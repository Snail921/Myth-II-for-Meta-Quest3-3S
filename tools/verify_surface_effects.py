"""Execute original stamp routine and compare all destination bytes/guards."""
from verify_combat_builders import *
from surface_effects import stamp_shade
import random

def main():
    rng=random.Random(434)
    c=machine()
    cases=0
    fixtures=[]
    for kind in (0,1,2):
        for case in range(160):
            size=16;w=rng.randrange(1,20);h=rng.randrange(1,20);stride=w+rng.randrange(3)
            x=rng.randrange(-500,1200);y=rng.randrange(-500,1200)
            strength=(0,1,32768,65535)[case%4]
            dst=bytes(rng.randrange(256) for _ in range(size*size))
            src=bytes(rng.randrange(256) for _ in range(stride*h))
            expected=bytearray(dst)
            stamp_shade(expected,size,src,w,h,stride,x,y,kind,strength)
            c.mem_write(0x100cf00,b'\xa5'*256+dst+b'\xa5'*256)
            c.mem_write(0x100e000,src)
            run(c,0x4bae80,[0x100d000,size,kind,strength,0x100e000,stride,h],
                {UC_X86_REG_EAX:x&0xffffffff,UC_X86_REG_ECX:w,UC_X86_REG_EDX:y&0xffffffff})
            actual=bytes(c.mem_read(0x100d000,size*size))
            assert actual==expected,(kind,case,x,y,w,h,strength)
            assert bytes(c.mem_read(0x100cf00,256))==b'\xa5'*256
            assert bytes(c.mem_read(0x100d100,256))==b'\xa5'*256
            fixtures.append(struct.pack("<8i",size,w,h,stride,x,y,kind,strength)+dst+src+actual)
            cases+=1
    (ROOT/'build/surface-oracle.bin').write_bytes(struct.pack('<I',cases)+b''.join(fixtures))
    report=dict(result='pass',routine='004bae80',cases=cases,
                modes=['shadow attenuation with floor','saturating light addition','model shadow minimum'],
                scope='random shade bytes, intensity, subpixel placement, clipping, padded source stride; original instructions, no stub inside routine',
                limitation='software shade compositor only; not OpenGL final-frame or live capture validation')
    (ROOT/'research/reports/surface-effects-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('PASS original terrain shade compositor:',cases,'cases, complete buffers and guards')

if __name__=='__main__':main()
