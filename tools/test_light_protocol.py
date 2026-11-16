import struct
from decode_bridge import decode

for version in range(1, 9):
    data = b'MYTHBR01' + struct.pack('<14I', version, 1, 1, 0, 0, 0, 216, 80, 0, 0, 0, 0, 0, 0)
    if version >= 2:
        data += bytes(272)
    if version >= 3:
        data += bytes(4)
    if version >= 4:
        data += bytes(4)
    if version >= 5:
        data += struct.pack('<QI', 1, 0)
    if version >= 6:
        block = struct.pack('<I16I', 1, 123, 2, 0x10001, 1, 2, 3, 0, 0, 2, 2, 65535, 0, 0, 0, 0, 0) + bytes([255]*4)
        data += struct.pack('<I', len(block)) + block
    if version>=7:
        data+=struct.pack('<I16I',1,123,1997670900,0,0,100,200,300,65535,256,256,0,0,0,1,0,0)
    result = decode(data)
    assert result['lights'] is None if version < 6 else len(result['lights']) == 1
    assert result['particles'] is None if version < 7 else len(result['particles']) == 1
    if version >= 7:
        for n in range(len(data)):
            try:
                decode(data[:n])
            except ValueError:
                pass
            else:
                raise AssertionError(n)
print('PASS snapshot v1-v8 compatibility and all truncated v7/v8 packets')
