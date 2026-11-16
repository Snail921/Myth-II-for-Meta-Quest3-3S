"""Execute original Build 434 shade-table code against our terrain conversion."""
import json, struct
from verify_combat_builders import machine, ROOT, STACK, STOP, UC_X86_REG_ESP, UC_X86_REG_EIP
from prepare_vr_assets import u, decode
from terrain_lighting import lighting_settings, shading_table

def main():
    raw=(ROOT/'build/scene-bridge/landscape-l1.collection').read_bytes()
    offset=u(raw,248)+u(raw,68)
    palette=[struct.unpack_from('>3H',raw,offset+32+i*8) for i in range(u(raw,offset))]
    settings=lighting_settings(ROOT/'research/fixtures/maps/willow-creek/decoded-map.json')
    cases=[settings,dict(settings,transition_point=32768,light_fraction=181,light_color=[65535,40000,16000]),dict(settings,transition_point=95027,dark_fraction=229,light_fraction=25)]
    results=[]
    for setting in cases:
        c=machine(); base=0x1100000; output=base+0x1000
        c.mem_map(base,0x42000)
        c.mem_write(base,struct.pack('<I',len(palette))+bytes(28)+b''.join(struct.pack('<3H2x',*p) for p in palette))
        c.mem_write(base+0x900,struct.pack('<3H',*setting['dark_color']))
        c.mem_write(base+0x910,struct.pack('<3H',*setting['light_color']))
        args=[3,256,base,output,setting['dark_fraction'],base+0x900,setting['transition_point'],setting['light_fraction'],base+0x910]
        c.mem_write(STACK,struct.pack('<10I',STOP,*args));c.reg_write(UC_X86_REG_ESP,STACK)
        c.emu_start(0x4c9a11,STOP,count=20000000)
        assert c.reg_read(UC_X86_REG_EIP)==STOP
        assert c.reg_read(UC_X86_REG_ESP)==STACK+4
        actual=bytes(c.mem_read(output,256*1024)); expected=shading_table(palette,setting)
        for shade in range(256):
            for index,rgba in enumerate(expected[shade]):
                b,g,r,_=actual[shade*1024+index*4:shade*1024+index*4+4]
                assert bytes((r,g,b,255))==rgba,(shade,index,rgba,(r,g,b))
        results.append(dict(transition=setting['transition_point'],palette_colors=len(palette),shade_levels=256,exact_match=True))
    # Index-preserving decode must keep each original color/light pair aligned.
    decoded,_,_=decode(raw); indexed,_,_=decode(raw,palette_indices=True)
    for tile in range(42):
        w,h,p=decoded[tile*2];iw,ih,indices=indexed[tile*2]
        assert (w,h)==(iw,ih)==(256,256)
        assert decoded[tile*2+1]==indexed[tile*2+1]
        for j in range(0,len(p),4):
            assert tuple(p[j:j+3])==tuple(v>>8 for v in palette[indices[j]])
    (ROOT/'research/reports/terrain-lighting-verification.json').write_text(json.dumps(results,indent=2))
    print(json.dumps(results))

if __name__=='__main__':main()
