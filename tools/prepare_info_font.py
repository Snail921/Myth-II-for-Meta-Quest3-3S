"""Original high-resolution screen font, selected by standard interface stin+8.
Format reference: pinned MIT mythextract font_tag.py. Preserve glyphs and advances.
"""
from pathlib import Path
import struct,os
from PIL import Image
from level_assets import inventory,raw_tag
root=Path(__file__).resolve().parents[1]
output=Path(os.environ.get('MYTH_SHARED_ASSETS',root/'build/vr/assets'));output.mkdir(parents=True,exist_ok=True)
tags=inventory()
font_key=raw_tag(tags,'inte','stin')[8:12].decode('latin1')
b=raw_tag(tags,'font',font_key)
flags,asc,desc,leading,spacing,count,pos,length=struct.unpack_from('>I4H3I',b)
assert count<=256 and pos+length==len(b) and asc+desc<=32
image=Image.new('RGBA',(512,512),(255,255,255,0))
metrics=[12]*256
for _ in range(count):
    code,advance,w,h,x,y=struct.unpack_from('>4H2h',b,pos)
    pos+=16
    assert code<256 and w<=24 and h<=32 and pos+w*h<=len(b)
    metrics[code]=max(1,advance+spacing)
    for yy in range(h):
        for xx in range(w):
            dx=xx-x+4;dy=asc-y+yy
            if 0<=dx<32 and 0<=dy<32:
                image.putpixel(((code%16)*32+dx,(code//16)*32+dy),(255,255,255,255-b[pos+yy*w+xx]))
    pos+=w*h+(w*h&1)
assert pos==len(b)
out=output/'info-font.rgba'
out.write_bytes(struct.pack('<II',512,512)+image.tobytes()+struct.pack('<256H',*metrics))
# Preview is not part of the deployment bundle.
print(f'Original font {font_key}: {count} glyphs, ascent={asc}, descent={desc}')

# Original status-bar decoration, untouched RGBA decoded from Internal Status Bar.
from prepare_vr_assets import decode
images,_,_=decode(raw_tag(tags,'.256','!sba'))
indices=[1,3,13,15,29,30,31,32,33,34,35,36,37,38,39,40,41]
pack=bytearray(struct.pack('<I',len(indices)+20))
for index in indices:
    w,h,pixels=images[index]
    pack+=struct.pack('<III',index,w,h)+bytes(pixels)
# Original control bar table 528fb8: actions 8..17 use collection sequences 2..11.
control,instances,sequences=decode(raw_tag(tags,'.256','!cba'))
for formation in range(10):
    for state in range(2):
        ref=sequences[formation+2][2][state][0]
        flags,_,_,bitmap=instances[ref]
        assert bitmap>=0 and not flags&3
        w,h,pixels=control[bitmap]
        assert (w,h)==(28,21)
        pack+=struct.pack('<III',100+formation*2+state,w,h)+bytes(pixels)
(output/'info-decoration.pack').write_bytes(pack)

# Original dialog and toolbar templates; displayed pixels come from the running game.
pack=bytearray()
for tag,index in (("!sba",13),("!sba",1),("ingh",0),("ingp",0),("!sba",14),("!sba",15),("!sba",16),("!sba",2),("!sba",3),("!sba",4)):
    ims,_,_=decode(raw_tag(tags,".256",tag))
    w,h,p=ims[index];pack+=struct.pack("<II",w,h)+bytes(p)
(output/"dialog-templates.pack").write_bytes(pack)
