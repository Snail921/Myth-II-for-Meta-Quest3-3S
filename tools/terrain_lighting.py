"""Original terrain shade-table blend (Build 434, 0x4c9a11).

Use full RGB16 palette values until the final RGB8 conversion. This is the
software renderer's integer blend, also consistent with the GL shader's
dark/light-color interpolation (0x489ed0), apart from its float rounding.
"""
import json, struct

def lighting_settings(path):
    h=json.loads(path.read_text())['header']
    return dict(dark_fraction=h['dark_fraction'],light_fraction=h['light_fraction'],
                dark_color=list(struct.unpack('>3H',h['dark_color'].encode('latin1')[:6])),
                light_color=list(struct.unpack('>3H',h['light_color'].encode('latin1')[:6])),
                transition_point=h['transition_point'],mesh_lighting_tag=h['mesh_lighting_tag'],global_tint_fraction=h['global_tint_fraction'],global_tint_color=h['global_tint_color'])

def global_tint_rgb16(rgb, fraction, color):
    # Original 0x4c9c18 param8/param9, display-encoded RGB16 arithmetic.
    intensity=((2*rgb[0]+3*rgb[1]+rgb[2])//5)*fraction>>16
    return [min(65535,(c*(256-fraction)+t*intensity)>>8) for c,t in zip(rgb,color)]

def tint_rgba(pixels, settings):
    fraction=settings.get('global_tint_fraction',0)
    if not fraction:return pixels
    color=struct.unpack('>3H',settings['global_tint_color'].encode('latin1')[:6]);out=bytearray(pixels)
    for i in range(0,len(out),4):out[i:i+3]=bytes(v>>8 for v in global_tint_rgb16([x*257 for x in out[i:i+3]],fraction,color))
    return out

def shading_table(palette, settings):
    transition=settings['transition_point']
    # Chimera uses 95027 (>1.0 fixed point). The original allows the pivot
    # beyond the shade table, leaving every entry on the dark branch.
    if not 0<=transition<=0x7fffff:raise ValueError('transition range')
    if any(not 0<=settings[k]<=256 for k in ('dark_fraction','light_fraction')):
        raise ValueError('shade fraction range')
    pivot=(255*transition)>>16
    result=[]
    for shade in range(256):
        if shade < pivot:
            fraction=(pivot-shade)*settings['dark_fraction']//pivot
            color=settings['dark_color']
        elif shade > pivot:
            fraction=(shade-pivot)*settings['light_fraction']//(255-pivot)
            color=settings['light_color']
        else:
            fraction=0; color=settings['dark_color']
        values=[[min(65535,(c*(256-fraction)+t*fraction)>>8) for c,t in zip(rgb,color)] for rgb in palette]
        global_fraction=settings.get('global_tint_fraction',0)
        if global_fraction:
            tint=struct.unpack('>3H',settings['global_tint_color'].encode('latin1')[:6])
            values=[global_tint_rgb16(rgb,global_fraction,tint) for rgb in values]
        result.append([bytes([v>>8 for v in rgb]+[255]) for rgb in values])
    return result
