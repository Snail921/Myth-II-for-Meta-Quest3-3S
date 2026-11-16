"""Recover bounded byte-swap plans from original Build 434 descriptor tables."""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile


def recover(pe, va):
    tokens = []
    cursor = va - pe.OPTIONAL_HEADER.ImageBase
    def read():
        nonlocal cursor
        token, = struct.unpack('<h',pe.get_data(cursor,2))
        cursor += 2
        tokens.append(token)
        if len(tokens)>512:
            raise ValueError('Descriptor budget exceeded')
        return token
    def block(depth=0):
        if depth>8 or read()!=-100:
            raise ValueError('Invalid descriptor start')
        repeats=read()
        if not 1<=repeats<=32767:
            raise ValueError('Invalid repeat count')
        offset, spans=0,[]
        while True:
            token=read()
            if token==-101:
                break
            if token==-100:
                nonlocal cursor
                cursor-=2
                tokens.pop()
                size,child=block(depth+1)
                spans.extend((offset+at,width) for at,width in child)
                offset+=size
            elif token in (-2,-4,-8):
                spans.append((offset,-token));offset-=token
            elif token>0:
                offset+=token
            else:
                raise ValueError(f'Unsupported descriptor token {token}')
        if offset*repeats>65536:
            raise ValueError('Descriptor extent too large')
        return offset*repeats,[(i*offset+at,width) for i in range(repeats) for at,width in spans]
    size,spans=block()
    return dict(va=va,size=size,spans=spans,tokens=tokens)


def main():
    data=(ROOT/'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    pe=pefile.PE(data=data)
    locations={'header':(0x529ab0,1024),'cell':(0x529a9c,12),'marker':(0x5298a8,64),
               'palette':(0x5298d0,32),'connector':(0x527a04,64),'marker_extra':(0x5298e8,16),
               'action':(0x52d3f8,64),'parameter':(0x52d418,8)}
    plans={}
    for name,(va,size) in locations.items():
        plan=recover(pe,va)
        assert plan['size']==size,(name,plan['size'])
        plans[name]=plan
    # Both type-specific paths use the same 16-byte swap plan at marker+36.
    extras=[]
    for kind in range(12):
        va,=struct.unpack('<I',pe.get_data(0x517118+kind*0x34-0x400000,4))
        if va:
            assert recover(pe,va)['spans']==plans['marker_extra']['spans']
            extras.append(kind)
    assert extras==[5,6]
    parameters=[]
    for kind in range(21):
        size,va=struct.unpack('<II',pe.get_data(0x52cee8+kind*8-0x400000,8))
        plan=recover(pe,va)
        assert plan['size']==size
        width=0 if kind in (0,1) else (2 if size==2 else 4)
        expected=[] if width==0 else [(offset,width) for offset in range(0,size,width)]
        assert plan['spans']==expected
        parameters.append(dict(type=kind,size=size,descriptor=va,swap_width=width))
    lines=['/* Generated from original Build 434 descriptors. Re-run tools/recover_mesh_layout.py. */']
    for name,plan in plans.items():
        lines.append(f'static const swap_span {name}_swaps[] = {{')
        lines.extend(f'    {{{at}, {width}}},' for at,width in plan['spans'])
        lines.append('};')
    lines.append('static const uint8_t parameter_sizes[21] = {'+','.join(str(p['size']) for p in parameters)+'};')
    lines.append('static const uint8_t parameter_widths[21] = {'+','.join(str(p['swap_width']) for p in parameters)+'};')
    (ROOT/'src/compat/mesh_layout.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    result=dict(original_sha256=hashlib.sha256(data).hexdigest(),plans=plans,marker_extra_types=extras,parameter_types=parameters,
                limitations='Descriptor-derived storage conversion only; runtime pointer binding and mission initialization are separate.')
    (ROOT/'research/reports/mesh-layout-recovery.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print({name:len(plan['spans']) for name,plan in plans.items()})


if __name__=='__main__':main()
