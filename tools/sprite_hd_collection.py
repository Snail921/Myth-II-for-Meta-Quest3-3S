"""Read Twice Born d256 bitmap references in their neutral (pre-team-color) palette."""
import struct
from pathlib import Path

import numpy as np
from PIL import Image


def references(raw):
    count, offset = struct.unpack_from('>II', raw, 8)
    start = 64 + offset
    if not 0 < count <= 65536 or start + count * 128 > len(raw):
        raise ValueError('Invalid HD bitmap table')
    for index in range(count):
        ref = raw[start + index*128:start + (index+1)*128]
        pos, size = struct.unpack_from('>II', ref, 64)
        if size < 52 or 64 + pos + size > len(raw):
            raise ValueError('Invalid HD bitmap reference')
        yield index, ref, memoryview(raw)[64+pos:64+pos+size]


def component_rgb(parts):
    # Match hd_codec.h hueRGB with neutral controls and unsigned 32-bit arithmetic.
    rgb = parts[:, [2, 1, 0]].astype(np.uint32)
    tagged = parts[:, 3] >= 32
    if tagged.any():
        hue, value, saturation = rgb[tagged].T
        hue = hue * 0x60606
        sector = hue >> 24
        packed = ((((((sector & 1) - 1) ^ hue) & 0xffffff) * saturation >> 8) & 0xff0000) | saturation
        packed = (packed ^ 0xff00ff) * value
        shift = (sector * 5) & 31
        color = (((packed >> 8) & 255) << ((0x10884000 >> shift) & 31)
                 | (packed >> 24) << ((0x1040208 >> shift) & 31)
                 | value << ((0x20002110 >> shift) & 31))
        rgb[tagged] = np.stack((color >> 16, (color >> 8) & 255, color & 255), axis=1)
    return rgb


def bitmap(data):
    width, height, _, flags = struct.unpack_from('>4H', data)
    encoding = struct.unpack_from('>H', data, 16)[0]
    if not (0 < width <= 4096 and 0 < height <= 4096 and flags & 16384):
        raise ValueError('Invalid HD dimensions or flags')
    size = width * height
    pos = 52 + (0 if flags & 32 else 4*(height-1))
    if encoding in (2, 3):
        mode = 'BGRA' if encoding == 2 else 'RGB'
        image = Image.frombytes('RGBA' if encoding == 2 else 'RGB', (width,height), bytes(data[pos:]), 'raw', mode)
        return image.convert('RGBA')
    if encoding != 1:
        raise ValueError(f'Unsupported HD encoding {encoding}')
    first, second = bytearray(size*4), bytearray(size*4)
    index = state = 0
    while index < size:
        if state & 31 in (0, 31):
            count = data[pos]
            pos += 1
            run_state, state = state, 1
        else:
            state = data[pos]
            pos += 1
            run_state, count = state, 1
        if index + count > size:
            raise ValueError('HD run exceeds image')
        if run_state & 31:
            for i in range(index*4, (index+count)*4, 4):
                first[i:i+3] = data[pos:pos+3]
                first[i+3] = run_state
                extra = data[pos] & 1
                pos += 3
                if extra:
                    second[i:i+4] = data[pos:pos+4]
                    pos += 4
        index += count
    primary = np.frombuffer(first, np.uint8).reshape(-1,4)
    secondary = np.frombuffer(second, np.uint8).reshape(-1,4)
    rgb = component_rgb(primary)
    mixed = secondary[:,3] != 0
    if mixed.any():
        other = component_rgb(secondary[mixed])
        weight = ((secondary[mixed,3:4].astype(np.uint32) & 31)*4)
        weight = weight[:,0]
        r, g, b = rgb[mixed].T
        rb = (r << 16) | b
        rb += ((((other[:,0] << 16) | other[:,2]) - rb)*weight) >> 8
        green = g << 8
        green += (((other[:,1] << 8) - green)*weight) >> 8
        rgb[mixed] = np.stack(((rb >> 16) & 255, (green >> 8) & 255, rb & 255), axis=1)
    rgba = np.empty((size,4), dtype=np.uint8)
    rgba[:,:3] = rgb
    rgba[:,3] = ((primary[:,3].astype(np.uint32) & 31)*1053) >> 7
    return Image.fromarray(rgba.reshape(height,width,4))


def first_image(path):
    # Gallery previews only need the first bitmap, even for large actors.
    with Path(path).open('rb') as src:
        header = src.read(64)
        count, offset = struct.unpack_from('>II', header, 8)
        if not count:
            raise ValueError('Empty HD collection')
        src.seek(64+offset)
        pos, size = struct.unpack_from('>II', src.read(128), 64)
        src.seek(64+pos)
        return bitmap(src.read(size))
