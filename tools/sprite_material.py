"""Portable sprite material projects; no game files or runtime IDs are required."""
import hashlib
import io
import json
import os
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def digest(image):
    return hashlib.sha256(str(image.size).encode() + image.tobytes()).hexdigest()


@dataclass
class Frame:
    name: str
    image: Image.Image
    identity: dict = field(default_factory=dict)
    mask: Image.Image = None
    correction: Image.Image = None
    correction_mask: Image.Image = None

    def __post_init__(self):
        self.image = self.image.convert('RGBA')
        if self.mask is None:
            self.mask = Image.new('L', self.image.size)
        self.identity = dict(self.identity, image_sha256=digest(self.image))


class Project:
    def __init__(self, frames=(), name='Sprites'):
        self.frames = list(frames)
        self.name = name
        self.colors = []
        self.tolerance = 30
        self.height_value = 128
        self.strength = 2.0
        self.blur = 0.0
        self.correction_enabled = True

    @classmethod
    def folder(cls, path, progress=lambda *args: None):
        root = Path(path)
        paths = sorted(p for p in root.rglob('*') if p.suffix.lower() in ('.png', '.tga', '.bmp'))
        if not paths:
            raise ValueError('No PNG/TGA/BMP images found')
        frames = []
        for i, p in enumerate(paths):
            with Image.open(p) as im:
                frames.append(Frame(p.relative_to(root).as_posix(), im.copy(), {'source_path': p.relative_to(root).as_posix()}))
            progress(i+1, len(paths))
        return cls(frames, root.name)

    @classmethod
    def collection(cls, path, tag=None, progress=lambda *args: None):
        from prepare_vr_assets import decode, resolved_instance
        raw = Path(path).read_bytes()
        images, instances, sequences = decode(raw)
        uses = {}
        for si, (flags, scale, rows) in enumerate(sequences):
            for fi, row in enumerate(rows):
                for vi in range(len(row)):
                    fl, rx, ry, bitmap = resolved_instance(images, instances, flags, row, vi)
                    if bitmap >= 0:
                        uses.setdefault(bitmap, []).append(dict(sequence=si, frame=fi, direction=vi, flags=fl, origin=[rx, ry], scale=scale))
        frames = []
        for index in sorted(uses):
            w, h, pixels = images[index]
            identity = dict(collection_tag=tag, collection_sha256=hashlib.sha256(raw).hexdigest(), bitmap=index, uses=uses[index])
            frames.append(Frame(f'bitmap-{index:04d}', Image.frombytes('RGBA', (w, h), bytes(pixels)), identity))
            progress(len(frames), len(uses))
        if not frames:
            raise ValueError('Collection has no sprite frames')
        return cls(frames, Path(path).stem)

    def color_mask(self, frame):
        rgba = np.asarray(frame.image).astype(np.int32)
        mask = np.zeros(rgba.shape[:2], dtype=bool)
        for color in self.colors:
            # RMS RGB distance: tolerance 0 is an exact match, 255 includes all.
            mask |= np.sum((rgba[:, :, :3]-np.asarray(color))**2, axis=2) <= 3*self.tolerance**2
        return Image.fromarray((mask & (rgba[:, :, 3] > 0)).astype(np.uint8)*255)

    def apply_masks(self, progress=lambda *args: None):
        for i, f in enumerate(self.frames):
            f.mask = self.color_mask(f)
            progress(i+1, len(self.frames))

    def height(self, frame):
        gray = frame.image.convert('L')
        if self.blur:
            # Alpha-normalized blur avoids sinking edges toward transparent black.
            a = frame.image.getchannel('A')
            premul = Image.fromarray((np.asarray(gray, dtype=np.float32)*np.asarray(a)/255).astype(np.uint8))
            weight = np.asarray(a.filter(ImageFilter.GaussianBlur(self.blur)), dtype=np.float32)
            value = np.asarray(premul.filter(ImageFilter.GaussianBlur(self.blur)), dtype=np.float32)
            gray = Image.fromarray(np.clip(value*255/np.maximum(weight, 1), 0, 255).astype(np.uint8))
        if self.correction_enabled and frame.correction is not None:
            gray = Image.composite(frame.correction, gray, frame.correction_mask)
        gray = Image.composite(Image.new('L', gray.size, self.height_value), gray, frame.mask)
        return gray

    def normal(self, frame, height=None):
        h = np.asarray(height if height is not None else self.height(frame), dtype=np.float32)/255
        visible = np.asarray(frame.image.getchannel('A')) > 0
        padded = np.pad(h, 1, mode='edge')
        alpha = np.pad(visible, 1, mode='edge')
        left = np.where(alpha[1:-1, :-2], padded[1:-1, :-2], h)
        right = np.where(alpha[1:-1, 2:], padded[1:-1, 2:], h)
        up = np.where(alpha[:-2, 1:-1], padded[:-2, 1:-1], h)
        down = np.where(alpha[2:, 1:-1], padded[2:, 1:-1], h)
        # Tangent +X right, +Y image-up, +Z toward viewer (OpenGL convention).
        n = np.stack((-(right-left)*.5*self.strength, (down-up)*.5*self.strength, np.ones_like(h)), axis=2)
        n /= np.linalg.norm(n, axis=2, keepdims=True)
        rgb = np.rint((n*.5+.5)*255).astype(np.uint8)
        rgb[~visible] = (128, 128, 255)
        return Image.fromarray(np.dstack((rgb, np.asarray(frame.image.getchannel('A')))), 'RGBA')

    def settings(self):
        return {k: getattr(self, k) for k in ('colors', 'tolerance', 'height_value', 'strength', 'blur', 'correction_enabled')}

    def save(self, path, progress=lambda *args: None):
        path = Path(path)
        fd, temp = tempfile.mkstemp(dir=path.parent, suffix='.tmp')
        os.close(fd)
        try:
            with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED) as z:
                rows = []
                for i, f in enumerate(self.frames):
                    rows.append(dict(name=f.name, identity=f.identity, correction=f.correction is not None))
                    layers=[('source', f.image), ('mask', f.mask)]
                    if f.correction is not None:
                        layers.extend([('correction',f.correction),('correction_mask',f.correction_mask)])
                    for kind, im in layers:
                        out = io.BytesIO(); im.save(out, format='PNG'); z.writestr(f'{kind}/{i}.png', out.getvalue())
                    progress(i+1, len(self.frames))
                z.writestr('project.json', json.dumps(dict(version=2, name=self.name, settings=self.settings(), frames=rows), ensure_ascii=False))
            os.replace(temp, path)
        finally:
            if os.path.exists(temp): os.unlink(temp)

    @classmethod
    def load(cls, path, progress=lambda *args: None):
        with zipfile.ZipFile(path) as z:
            if sum(i.file_size for i in z.infolist()) > 2*1024**3:
                raise ValueError('Project exceeds 2 GiB uncompressed')
            data = json.loads(z.read('project.json'))
            if data['version'] not in (1,2): raise ValueError('Unsupported project version')
            frames = []
            for i, row in enumerate(data['frames']):
                with Image.open(io.BytesIO(z.read(f'source/{i}.png'))) as im, Image.open(io.BytesIO(z.read(f'mask/{i}.png'))) as mask:
                    if im.size != mask.size: raise ValueError('Mask dimensions differ')
                    if digest(im.convert('RGBA')) != row['identity']['image_sha256']: raise ValueError('Source image hash mismatch')
                    frames.append(Frame(row['name'], im.copy(), row['identity'], mask.convert('L')))
                if row.get('correction',False):
                    for kind in ('correction','correction_mask'):
                        with Image.open(io.BytesIO(z.read(f'{kind}/{i}.png'))) as layer:
                            if layer.size != frames[-1].image.size:raise ValueError('Correction dimensions differ')
                            setattr(frames[-1],kind,layer.convert('L'))
                progress(i+1, len(data['frames']))
        result = cls(frames, data['name'])
        for k in result.settings():
            if k in data['settings']:setattr(result, k, data['settings'][k])
        if not (0 <= result.height_value <= 255 and 0 <= result.tolerance <= 255 and 0 <= result.strength <= 100 and 0 <= result.blur <= 20):
            raise ValueError('Invalid settings')
        return result

    def export_runtime(self, path, progress=lambda *args: None):
        """Exact-source-bound normal pack. Loaded from Quest files/SpriteMaterials."""
        import struct
        path = Path(path)
        fd, temp = tempfile.mkstemp(dir=path.parent, suffix='.tmp'); os.close(fd)
        try:
            with open(temp, 'wb') as out:
                out.write(b'MVRNORM1'); out.write(struct.pack('<I', len(self.frames)))
                for i, frame in enumerate(self.frames):
                    if max(frame.image.size)>2048:raise ValueError('Runtime material exceeds 2048 pixels')
                    out.write(struct.pack('<II', *frame.image.size))
                    out.write(frame.image.tobytes());out.write(self.normal(frame).tobytes())
                    progress(i+1,len(self.frames))
                if out.tell()>64*1024*1024:raise ValueError('Runtime material exceeds 64 MiB')
            os.replace(temp,path)
        finally:
            if os.path.exists(temp):os.unlink(temp)

    def export(self, path, progress=lambda *args: None):
        # A ZIP avoids partial output trees and preserves any previous good export.
        path = Path(path)
        fd, temp = tempfile.mkstemp(dir=path.parent, suffix='.tmp'); os.close(fd)
        try:
            with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED) as z:
                rows = []
                for i, f in enumerate(self.frames):
                    height = self.height(f)
                    rgba = Image.merge('RGBA', (height, height, height, f.image.getchannel('A')))
                    for kind, im in [('height', rgba), ('normal', self.normal(f, height)), ('mask', f.mask)]:
                        out = io.BytesIO(); im.save(out, format='PNG'); z.writestr(f'{kind}/{i:05d}.png', out.getvalue())
                    rows.append(dict(index=i, name=f.name, identity=f.identity))
                    progress(i+1, len(self.frames))
                z.writestr('manifest.json', json.dumps(dict(version=1, name=self.name, normal_convention='tangent-x-right-y-up-z-viewer', runtime_ready=False, settings=self.settings(), frames=rows), ensure_ascii=False))
            os.replace(temp, path)
        finally:
            if os.path.exists(temp): os.unlink(temp)
