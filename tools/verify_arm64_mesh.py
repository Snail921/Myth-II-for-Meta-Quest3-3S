"""Compare ARM64 mesh storage conversion with original x86 conversion routines.

No world pointers are bound and no mission actions are executed by this test.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/python-deps'))
import pefile
import unicorn as u
from unicorn import x86_const as x
from unicorn import arm64_const as a
from verify_arm64_compat import load_arm64


def main():
    fixture=ROOT/'research/fixtures/maps/willow-creek'
    raw=(fixture/'mesh.bin').read_bytes()
    decoded=json.loads((fixture/'decoded-map.json').read_text(encoding='utf-8'))
    executable=(ROOT/'research/extracted/update-1.8.1/Myth II.exe').read_bytes()
    library=(ROOT/'build/compat-arm64/libmyth181_compat.so').read_bytes()
    manifest=json.loads((ROOT/'build/compat-arm64/build-manifest.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(library).hexdigest()==manifest['binary_sha256']
    for file,digest in manifest['sources'].items():
        assert hashlib.sha256((ROOT/file).read_bytes()).hexdigest()==digest
    pe=pefile.PE(data=executable)
    cpu=u.Uc(u.UC_ARCH_X86,u.UC_MODE_32)
    cpu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
    cpu.mem_write(0x400000,pe.get_memory_mapped_image())
    cpu.mem_map(0x1000000,0x20000)
    cpu.mem_map(0x2000000,0x200000)
    base=0x2000010
    cpu.mem_write(base-16,b'\xa5'*16+raw+b'\xa5'*16)
    stack,stop=0x101f000,0x1000000

    def invoke(entry, count, pointer, args=(), edx=0):
        cpu.mem_write(stack,struct.pack('<'+'I'*(len(args)+1),stop,*args))
        cpu.reg_write(x.UC_X86_REG_ESP,stack)
        cpu.reg_write(x.UC_X86_REG_EAX,count)
        cpu.reg_write(x.UC_X86_REG_ECX,pointer)
        cpu.reg_write(x.UC_X86_REG_EDX,edx)
        cpu.emu_start(entry,stop,count=20000000)
        assert cpu.reg_read(x.UC_X86_REG_EIP)==stop
        assert cpu.reg_read(x.UC_X86_REG_ESP)==stack+4
    sections=decoded['sections'];header=decoded['header']
    invoke(0x481677,1,base,(0x51a1d8,1024,0x529ab0))
    invoke(0x481677,decoded['cell_count'],base+sections['mesh']['offset'],(0x51a1d8,12,0x529a9c))
    invoke(0x434574,header['marker_palette_entries'],base+sections['marker_palette']['offset'])
    invoke(0x43451e,header['marker_count'],base+sections['markers']['offset'])
    invoke(0x481677,header['connector_count'],base+sections['connectors']['offset'],(0x51a1d8,64,0x527a04))
    for name in ('media_coverage_region','mesh_LOD_data'):
        invoke(0x4816a4,0,0xfffe,(0x51a1d8,sections[name]['size']//2),base+sections[name]['offset'])
    actions_at=sections['map_actions']['offset']
    invoke(0x481677,header['map_action_count'],base+actions_at,(0x51a1d8,64,0x52d3f8))
    parameter_cursor=header['map_action_count']*64
    parameter_checks=0
    while parameter_cursor<sections['map_actions']['size']:
        at=actions_at+parameter_cursor
        kind,count=struct.unpack_from('>HH',raw,at)
        stride,descriptor=struct.unpack('<II',pe.get_data(0x52cee8+kind*8-0x400000,8))
        invoke(0x481677,1,base+at,(0x51a1d8,8,0x52d418))
        if count:
            invoke(0x481677,count,base+at+8,(0x51a1d8,stride,descriptor))
        parameter_cursor+=8+((count*stride+3)&~3)
        parameter_checks+=1
    expected=bytes(cpu.mem_read(base,len(raw)))
    assert cpu.mem_read(base-16,16)==b'\xa5'*16
    assert cpu.mem_read(base+len(raw),16)==b'\xa5'*16

    arm,symbols=load_arm64(library)
    arm.mem_map(0x4000000,0x400000)
    source,output=0x4000010,0x4200010
    arm.mem_write(source,raw)
    def convert(data,declared_size=None,capacity=None):
        arm.mem_write(source,data)
        arm.mem_write(output-16,b'\xa5'*(len(raw)+32))
        for reg,value in [(a.UC_ARM64_REG_X0,source),(a.UC_ARM64_REG_X1,len(data) if declared_size is None else declared_size),
                          (a.UC_ARM64_REG_X2,output),(a.UC_ARM64_REG_X3,len(raw) if capacity is None else capacity),
                          (a.UC_ARM64_REG_SP,0x301f000),(a.UC_ARM64_REG_LR,0x3000000)]:
            arm.reg_write(reg,value)
        arm.emu_start(symbols['myth181_mesh_decode'],0x3000000,count=30000000)
        assert arm.reg_read(a.UC_ARM64_REG_PC)==0x3000000
        assert arm.reg_read(a.UC_ARM64_REG_SP)==0x301f000
        return arm.reg_read(a.UC_ARM64_REG_X0)
    assert convert(raw)==1
    actual=bytes(arm.mem_read(output,len(raw)))
    assert actual==expected, next((i for i,(lhs,rhs) in enumerate(zip(actual,expected)) if lhs!=rhs),None)
    assert arm.mem_read(output-16,16)==b'\xa5'*16
    assert arm.mem_read(output+len(raw),16)==b'\xa5'*16
    (fixture/'mesh-native-storage.bin').write_bytes(actual)
    cases=[('truncated_header',raw,1023,None),('small_output',raw,None,len(raw)-1),('truncated_body',raw,2048,None)]
    for name,offset,value in [('oversized_data',0x1c,0xffffffff),('bad_cell_size',0x10,12),
                              ('section_outside',0x28,0xffffffff),('overlap',0x28,0),('huge_marker_count',0x34,0xffffffff)]:
        edited=bytearray(raw);struct.pack_into('>I',edited,offset,value);cases.append((name,bytes(edited),None,None))
    edited=bytearray(raw);struct.pack_into('>H',edited,sections['markers']['offset']+4,12)
    cases.append(('unknown_marker_type',bytes(edited),None,None))
    at=sections['map_actions']['offset']
    for name,offset,value,fmt in [('action_outside',at+24,0xffffffff,'>I'),
                                  ('unknown_parameter',at+header['map_action_count']*64,21,'>H'),
                                  ('oversized_parameter',at+header['map_action_count']*64+2,65535,'>H')]:
        edited=bytearray(raw);struct.pack_into(fmt,edited,offset,value)
        cases.append((name,bytes(edited),None,None))
    for name,data,size,capacity in cases:
        assert convert(data,size,capacity)==0,name
        assert arm.mem_read(output-16,len(raw)+32)==b'\xa5'*(len(raw)+32),name
    # Preserve the original iteration order that feeds actor initialization.
    cpu.mem_write(0x529bd4,struct.pack('<I',base))
    for offset,name in ((0x30,'marker_palette'),(0x40,'markers')):
        cpu.mem_write(base+offset,struct.pack('<I',base+sections[name]['offset']))
    arm.mem_write(output,actual)
    iterator_checks=0
    marker_orders={}
    def compare_iterator(kind,state,record_count,section,stride):
        nonlocal iterator_checks
        xstate,astate=0x1005000,0x3005000
        cpu.mem_write(xstate-8,b'\xa5'*8+state+b'\xa5'*8)
        arm.mem_write(astate-8,b'\xa5'*8+state+b'\xa5'*8)
        xrecords=base+sections[section]['offset'];arecords=output+sections[section]['offset']
        order=[]
        for _ in range(record_count+1):
            cpu.mem_write(stack,struct.pack('<I',stop));cpu.reg_write(x.UC_X86_REG_ESP,stack)
            cpu.reg_write(x.UC_X86_REG_EDX if kind=='marker' else x.UC_X86_REG_ECX,xstate)
            cpu.emu_start(0x43435f if kind=='marker' else 0x4343f9,stop,count=200000)
            assert cpu.reg_read(x.UC_X86_REG_EIP)==stop
            assert cpu.reg_read(x.UC_X86_REG_ESP)==stack+4
            expected_pointer=cpu.reg_read(x.UC_X86_REG_EAX)
            for reg,value in [(a.UC_ARM64_REG_X0,arecords),(a.UC_ARM64_REG_X1,record_count),(a.UC_ARM64_REG_X2,astate),
                              (a.UC_ARM64_REG_SP,0x301f000),(a.UC_ARM64_REG_LR,0x3000000)]:arm.reg_write(reg,value)
            arm.emu_start(symbols['myth181_'+kind+'_next'],0x3000000,count=200000)
            assert arm.reg_read(a.UC_ARM64_REG_PC)==0x3000000
            assert arm.reg_read(a.UC_ARM64_REG_SP)==0x301f000
            pointer=arm.reg_read(a.UC_ARM64_REG_X0)
            assert (expected_pointer-xrecords if expected_pointer else None)==(pointer-arecords if pointer else None)
            assert cpu.mem_read(xstate-8,len(state)+16)==arm.mem_read(astate-8,len(state)+16)
            assert arm.mem_read(astate-8,8)==b'\xa5'*8
            assert arm.mem_read(astate+len(state),8)==b'\xa5'*8
            iterator_checks+=1
            if not pointer:return order
            order.append((pointer-arecords)//stride)
        raise AssertionError('Iterator did not terminate')
    for kind in range(-1,12):
        state=struct.pack('<5h',kind,-1,-1,0,0)
        marker_orders[str(kind)]=compare_iterator('marker',state,header['marker_count'],'markers',64)
        compare_iterator('palette',struct.pack('<I4h',0xffffffff,kind,-1,-1,0),header['marker_palette_entries'],'marker_palette',32)
    for palette,datum in ((1,-1),(999,-1),(-1,0),(-1,999)):
        compare_iterator('marker',struct.pack('<5h',3,palette,datum,0,0),header['marker_count'],'markers',64)
    for tag in (0x77616d31,0x706967ff,0x5a5a5a5a):
        compare_iterator('palette',struct.pack('<I4h',tag,3,-1,-1,0),header['marker_palette_entries'],'marker_palette',32)
    report=dict(original_sha256=hashlib.sha256(executable).hexdigest(),arm64_sha256=hashlib.sha256(library).hexdigest(),
                iterator_comparisons=iterator_checks,marker_order_counts={kind:len(order) for kind,order in marker_orders.items()},
                fixture_sha256=hashlib.sha256(raw).hexdigest(),converted_sha256=hashlib.sha256(actual).hexdigest(),
                compared_bytes=len(raw),cell_count=decoded['cell_count'],markers=header['marker_count'],palette_entries=header['marker_palette_entries'],
                connectors=header['connector_count'],actions=header['map_action_count'],parameters=parameter_checks,guard_cases=[c[0] for c in cases],result='pass',
                limitations='Storage conversion only. Runtime pointers unbound, actions decoded but not executed, no actors initialized or game ticks executed. Added rejection guards are not original equivalence claims.')
    (ROOT/'research/reports/arm64-mesh-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'PASS: {len(raw)} bytes of original x86/ARM64 mesh conversion match; {len(cases)} rejection guards; {iterator_checks} iterator comparisons.')


if __name__=='__main__':main()
