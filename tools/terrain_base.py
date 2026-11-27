"""Export unlit terrain color plus palette for persistent surface updates."""
import struct
from pathlib import Path
from PIL import Image
from terrain_lighting import global_tint_rgb16

def export_terrain_base(collection, images, palette, settings, width, height, out):
    colors=[]
    fraction=settings.get('global_tint_fraction',0)
    tint=struct.unpack('>3H',settings['global_tint_color'].encode('latin1')[:6])
    for rgb in palette:
        if fraction:rgb=global_tint_rgb16(rgb,fraction,tint)
        colors.append(bytes([v>>8 for v in rgb]+[255]))
    if len(colors)!=256:raise ValueError('terrain base palette')
    image=Image.new('RGBA',(width,height));cols=width//256
    for i in range((width//256)*(height//256)):
        w,h,p=images[2*i]
        if (w,h)!=(256,256):raise ValueError('terrain base tile')
        pixels=b''.join(colors[index] for index in p[0::4])
        image.paste(Image.frombytes('RGBA',(w,h),pixels),((i%cols)*256,(i//cols)*256))
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    (out/'terrain-base.pack').write_bytes(b'MYTHBC01'+struct.pack('<II',width,height)+b''.join(colors)+image.tobytes())
    image.save(out/'terrain-base.png')
    return image

if __name__=='__main__':
    import argparse,json
    from prepare_vr_assets import decode,u
    from terrain_lighting import lighting_settings
    parser=argparse.ArgumentParser();parser.add_argument('--work',required=True);parser.add_argument('--out',required=True);args=parser.parse_args()
    work=Path(args.work);collection=(work/'scene/landscape-l1.collection').read_bytes();images=decode(collection,palette_indices=True)[0]
    fixture=work/'decoded-map.json';meta=json.loads(fixture.read_text(encoding='utf8'));c=u(collection,248)+u(collection,68)
    palette=[struct.unpack_from('>3H',collection,c+32+i*8) for i in range(u(collection,c))]
    export_terrain_base(collection,images,palette,lighting_settings(fixture),meta['width']*8,meta['height']*8,args.out)
    print('Exported terrain-base.pack and terrain-base.png:',args.out)
