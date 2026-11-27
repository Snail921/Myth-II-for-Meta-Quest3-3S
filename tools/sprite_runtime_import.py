"""Import a Quest pack with its original source colours and coverage."""
import hashlib
import struct
from pathlib import Path

import numpy as np
from PIL import Image

from sprite_material import Frame, Project


def find_source(pack):
    data = Path(__file__).resolve().parents[1] / 'UserData'
    for folder in (pack.parent, data / 'UniqueCollections', data / 'HD Collections'):
        for ext in ('.bin', '.d256'):
            source = folder / (pack.stem.removesuffix('--unshadowed') + ext)
            if source.is_file():
                return source


def import_pack(pack, source=None, progress=lambda *args: None):
    pack = Path(pack)
    if not 12 <= pack.stat().st_size <= 64 * 1024 * 1024:
        raise ValueError('Invalid Quest pack size')
    with pack.open('rb') as stream:
        def read(count):
            block = stream.read(count)
            if len(block) != count:
                raise ValueError('Truncated Quest pack')
            return block

        magic = read(8)
        if magic not in (b'MVRNORM1', b'MVRNORM2', b'MVRMAT03', b'MVRMAT04', b'MVRMAT05', b'MVRMAT06'):
            raise ValueError('Unknown Quest pack format')
        count, = struct.unpack('<I', read(4))
        if not 1 <= count <= 10000:
            raise ValueError('Invalid frame count')
        project = Project([], pack.stem)
        if magic != b'MVRNORM1':
            if source is None:
                raise ValueError('Select the original sprite collection')
            project = Project.collection(source)
            if len(project.frames) != count:
                raise ValueError('Source collection frame count differs')
        for index in range(count):
            size = struct.unpack('<II', read(8))
            if min(size) < 1 or max(size) > 2048:
                raise ValueError('Invalid frame dimensions')
            length = size[0] * size[1] * 4
            packed = magic == b'MVRMAT03'
            if magic in (b'MVRMAT04', b'MVRMAT05', b'MVRMAT06'):
                flag, = struct.unpack('<I', read(4))
                if flag > 1:
                    raise ValueError('Invalid PBR flag')
                packed = bool(flag)
            if magic == b'MVRNORM1':
                frame = Frame(f'bitmap-{index:04d}', Image.frombytes('RGBA', size, read(length)))
                project.frames.append(frame)
            else:
                frame = project.frames[index]
                fingerprint = hashlib.sha256(struct.pack('<II', *size) + frame.image.tobytes()).digest()
                if frame.image.size != size or read(32) != fingerprint:
                    raise ValueError(f'Original source mismatch at bitmap {index}')
            normal = Image.frombytes('RGBA', size, read(length))
            if magic == b'MVRMAT06':
                from sprite_material_variants import runtime_hash, replace_albedo
                project.variant = 'unshadowed'
                frame.reference_image = Image.frombytes('RGBA', size, read(length))
                if runtime_hash(frame.reference_image) != fingerprint.hex(): raise ValueError('Reference image mismatch')
                frame.identity['source_runtime_sha256'] = fingerprint.hex()
                replace_albedo(project, index, Image.frombytes('RGBA', size, read(length)))
            if packed:
                red, green, frame.thickness, alpha = normal.split()
                bits = np.asarray(alpha).astype(np.uint16)
                if magic in (b'MVRMAT05', b'MVRMAT06'):
                    frame.metallic = Image.fromarray(((bits >> 7) * 255).astype('uint8'))
                    frame.metallic_smoothness = Image.fromarray((((bits & 127) * 255 + 63) // 127).astype('uint8'))
                else:
                    frame.metallic_smoothness = alpha
                    frame.metallic = alpha.point(lambda v: 255 if v >= 128 else 0)
                x, y = (np.asarray(channel, dtype=float) / 127.5 - 1 for channel in (red, green))
                z = np.sqrt(np.maximum(0, 1 - x*x - y*y))
                normal = Image.merge('RGBA', (red, green, Image.fromarray(np.rint((z+1)*127.5).astype('uint8')), alpha))
            normal.putalpha(255)
            frame.normal_ink = normal
            frame.pbr_enabled = packed
            progress(index+1, count)
        if stream.read(1):
            raise ValueError('Unexpected trailing Quest pack data')
    if project.variant == 'unshadowed':
        from sprite_material_variants import collection_id, validate
        project.source_collection_id = collection_id(project)
        validate(project)
    return project

