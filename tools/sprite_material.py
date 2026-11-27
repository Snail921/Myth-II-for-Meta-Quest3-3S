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
    metallic_smoothness: Image.Image = None
    thickness: Image.Image = None
    pbr_enabled: bool = False
    metallic: Image.Image = None

    height_ink: Image.Image = None
    normal_ink: Image.Image = None
    selection: Image.Image = None
    selection_active: bool = False
    normal_inverted: bool = False

    def edit_mask(self):
        if self.selection_active:
            return self.selection if self.selection is not None else Image.new('L',self.image.size)
        return self.image.getchannel('A').point(lambda a:255 if a else 0)

    def __post_init__(self):
        self.image = self.image.convert('RGBA')
        if self.mask is None:
            self.mask = Image.new('L', self.image.size)
        for kind in ('metallic_smoothness', 'thickness', 'metallic'):
            layer = getattr(self, kind)
            if layer is None: layer = Image.new('L', self.image.size, 255 if kind == 'thickness' else 0)
            if layer.size != self.image.size: raise ValueError('Material layer dimensions differ')
            setattr(self, kind, layer.convert('L'))
        self.identity = dict(self.identity, image_sha256=digest(self.image))


class Project:
    def __init__(self, frames=(), name='Sprites'):
        self.frames = list(frames)
        self.name = name
        self.variant = 'shadowed'
        from sprite_material_variants import collection_id
        self.source_collection_id = collection_id(self)
        self.colors = []
        self.history = []
        from sprite_material_hints import HINT_COUNT, empty_hint
        self.material_hints = [empty_hint() for _ in range(HINT_COUNT)]
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
        if Path(path).suffix.lower() == '.d256':
            from sprite_hd_collection import bitmap, references
            frames = []
            collection_hash = hashlib.sha256(raw).hexdigest()
            refs = list(references(raw))
            for index, ref, data in refs:
                identity = dict(collection_tag=raw[4:8].hex(), collection_sha256=collection_hash,
                                bitmap=index, hd_reference=ref[84:108].hex())
                frames.append(Frame(f'bitmap-{index:04d}', bitmap(data), identity))
                progress(index+1, len(refs))
            return cls(frames, Path(path).stem)
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

    def view_image(self, frame, mode):
        if mode == 'source':
            return frame.image
        if mode == 'normal':
            return self.normal(frame)
        if mode == 'mask':
            overlay = Image.new('RGBA', frame.image.size, (255, 40, 70, 0))
            overlay.putalpha(frame.mask.point(lambda value: value // 2))
            return Image.alpha_composite(frame.image, overlay)
        gray = self.height(frame) if mode == 'height' else getattr(frame, mode)
        return Image.merge('RGBA', (gray, gray, gray, frame.image.getchannel('A')))

    def color_mask(self, frame, colors=None, tolerance=None, image=None):
        rgba = np.asarray(frame.image if image is None else image).astype(np.int32)
        mask = np.zeros(rgba.shape[:2], dtype=bool)
        for color in (self.colors if colors is None else colors):
            # RMS RGB distance: tolerance 0 is an exact match, 255 includes all.
            mask |= np.sum((rgba[:, :, :3]-np.asarray(color))**2, axis=2) <= 3*(self.tolerance if tolerance is None else tolerance)**2
        return Image.fromarray((mask & (np.asarray(frame.image.getchannel('A')) > 0)).astype(np.uint8)*255)

    def apply_masks(self, progress=lambda *args: None, indices=None):
        targets=range(len(self.frames)) if indices is None else indices
        for i,index in enumerate(targets):
            f=self.frames[index]
            f.mask = self.color_mask(f)
            progress(i+1, len(targets))

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
        if frame.height_ink is not None:gray.paste(frame.height_ink.convert("L"),mask=frame.height_ink.getchannel("A"))
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
        normal=Image.fromarray(np.dstack((rgb, np.asarray(frame.image.getchannel('A')))), 'RGBA')
        if frame.normal_ink is not None:
            normal.paste(frame.normal_ink,mask=frame.normal_ink.getchannel('A'));normal.putalpha(frame.image.getchannel('A'))
        if frame.normal_inverted:
            r,g,b,a=normal.split()
            normal=Image.merge('RGBA',(r.point(lambda v:255-v),g.point(lambda v:255-v),b,a))
        return normal

    def settings(self):
        return {k: getattr(self, k) for k in ('colors', 'tolerance', 'height_value', 'strength', 'blur', 'correction_enabled', 'material_hints')}

    def save(self, path, progress=lambda *args: None, recovery=None):
        from sprite_material_variants import validate, protect_target
        validate(self); protect_target(self, path, recovery=recovery is not None)
        path = Path(path)
        fd, temp = tempfile.mkstemp(dir=path.parent, suffix='.tmp')
        os.close(fd)
        try:
            with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED) as z:
                rows = []
                for i, f in enumerate(self.frames):
                    rows.append(dict(name=f.name, identity=f.identity, correction=f.correction is not None, pbr_enabled=f.pbr_enabled, normal_inverted=f.normal_inverted))
                    layers=[('source', f.image), ('mask', f.mask), ('metallic_smoothness', f.metallic_smoothness), ('thickness', f.thickness), ('metallic', f.metallic)]
                    if self.variant == 'unshadowed': layers.append(('reference', f.reference_image))
                    if f.correction is not None:
                        layers.extend([('correction',f.correction),('correction_mask',f.correction_mask)])
                    for kind in ('height_ink','normal_ink'):
                        if getattr(f,kind) is not None:layers.append((kind,getattr(f,kind)))
                    for kind, im in layers:
                        out = io.BytesIO(); im.save(out, format='PNG'); z.writestr(f'{kind}/{i}.png', out.getvalue())
                    progress(i+1, len(self.frames))
                z.writestr('project.json', json.dumps(dict(version=8, variant=self.variant, source_collection_id=self.source_collection_id, name=self.name, settings=self.settings(), frames=rows), ensure_ascii=False))
                z.writestr('edit-history.json', json.dumps(self.history, ensure_ascii=False))
                if recovery is not None: z.writestr('editor-session.json', json.dumps(recovery))
            os.replace(temp, path)
        finally:
            if os.path.exists(temp): os.unlink(temp)

    @classmethod
    def load(cls, path, progress=lambda *args: None):
        with zipfile.ZipFile(path) as z:
            if sum(i.file_size for i in z.infolist()) > 2*1024**3:
                raise ValueError('Project exceeds 2 GiB uncompressed')
            data = json.loads(z.read('project.json'))
            history = json.loads(z.read('edit-history.json')) if 'edit-history.json' in z.namelist() else []
            if data['version'] not in (1,2,3,4,5,6,7,8): raise ValueError('Unsupported project version')
            frames = []
            for i, row in enumerate(data['frames']):
                with Image.open(io.BytesIO(z.read(f'source/{i}.png'))) as im, Image.open(io.BytesIO(z.read(f'mask/{i}.png'))) as mask:
                    if im.size != mask.size: raise ValueError('Mask dimensions differ')
                    if digest(im.convert('RGBA')) != row['identity']['image_sha256']: raise ValueError('Source image hash mismatch')
                    frames.append(Frame(row['name'], im.copy(), row['identity'], mask.convert('L')))
                if data.get('variant') == 'unshadowed':
                    with Image.open(io.BytesIO(z.read(f'reference/{i}.png'))) as original:
                        frames[-1].reference_image = original.convert('RGBA')
                inverted=row.get('normal_inverted',False)
                if type(inverted) is not bool:raise ValueError('Invalid normal inversion flag')
                frames[-1].normal_inverted=inverted
                enabled=row.get('pbr_enabled', False)
                if type(enabled) is not bool: raise ValueError('Invalid PBR flag')
                frames[-1].pbr_enabled=enabled
                if data['version'] >= 3:
                    for kind in (('metallic_smoothness','thickness','metallic') if data['version']>=5 else ('metallic_smoothness','thickness')):
                        with Image.open(io.BytesIO(z.read(f'{kind}/{i}.png'))) as layer:
                            if layer.size != frames[-1].image.size: raise ValueError('Material layer dimensions differ')
                            setattr(frames[-1], kind, layer.convert('L'))
                if data['version']<5:
                    frames[-1].metallic=frames[-1].metallic_smoothness.point(lambda v:255 if v>=128 else 0)
                for kind in ('height_ink','normal_ink'):
                    member=f'{kind}/{i}.png'
                    if member in z.NameToInfo:
                        with Image.open(io.BytesIO(z.read(member))) as im:
                            if im.size!=frames[-1].image.size:raise ValueError('Paint layer dimensions differ')
                            setattr(frames[-1],kind,im.convert('RGBA'))
                if row.get('correction',False):
                    for kind in ('correction','correction_mask'):
                        with Image.open(io.BytesIO(z.read(f'{kind}/{i}.png'))) as layer:
                            if layer.size != frames[-1].image.size:raise ValueError('Correction dimensions differ')
                            setattr(frames[-1],kind,layer.convert('L'))
                progress(i+1, len(data['frames']))
        result = cls(frames, data['name'])
        result.variant = data.get('variant', 'shadowed')
        result.source_collection_id = data.get('source_collection_id') or result.source_collection_id
        from sprite_material_variants import validate
        validate(result)
        result.history = history
        for k in result.settings():
            if k in data['settings']:setattr(result, k, data['settings'][k])
        if not (0 <= result.height_value <= 255 and 0 <= result.tolerance <= 255 and 0 <= result.strength <= 100 and 0 <= result.blur <= 20):
            raise ValueError('Invalid settings')
        from sprite_material_hints import validate_hints
        result.material_hints = validate_hints(result.material_hints)
        return result

    def export_runtime(self, path, progress=lambda *args: None):
        """V5: M1S7 alpha, per-image flags, then packed PBR or ordinary normal RGBA8."""
        import struct
        from sprite_material_variants import validate, protect_target
        validate(self); protect_target(self, path, runtime=True)
        variant = self.variant == 'unshadowed'
        path = Path(path)
        fd, temp = tempfile.mkstemp(dir=path.parent, suffix='.tmp'); os.close(fd)
        try:
            with open(temp, 'wb') as out:
                if not 1 <= len(self.frames) <= 10000: raise ValueError('Invalid material frame count')
                out.write(b'MVRMAT06' if variant else b'MVRMAT05'); out.write(struct.pack('<I', len(self.frames)))
                for i, frame in enumerate(self.frames):
                    if max(frame.image.size)>2048:raise ValueError('Runtime material exceeds 2048 pixels')
                    out.write(struct.pack('<III', *frame.image.size, int(frame.pbr_enabled)))
                    out.write(bytes.fromhex(frame.identity['source_runtime_sha256']) if variant else hashlib.sha256(struct.pack('<II', *frame.image.size)+frame.image.tobytes()).digest())
                    normal = self.normal(frame)
                    metal = (np.asarray(frame.metallic) >= 128).astype(np.uint8)
                    smooth = np.asarray(frame.metallic_smoothness, dtype=np.uint16)
                    alpha = (metal << 7) | ((smooth * 127 + 127) // 255).astype(np.uint8)
                    packed = Image.merge('RGBA', (normal.getchannel('R'), normal.getchannel('G'), frame.thickness, Image.fromarray(alpha)))
                    out.write((packed if frame.pbr_enabled else normal).tobytes())
                    if variant:
                        out.write(frame.reference_image.tobytes())
                        out.write(frame.image.tobytes())
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
                    for kind, im in [('albedo', f.image), ('height', rgba), ('normal', self.normal(f, height)), ('mask', f.mask), ('metallic_smoothness', f.metallic_smoothness), ('thickness', f.thickness), ('metallic', f.metallic)]:
                        out = io.BytesIO(); im.save(out, format='PNG'); z.writestr(f'{kind}/{i:05d}.png', out.getvalue())
                    rows.append(dict(index=i, name=f.name, identity=f.identity, pbr_enabled=f.pbr_enabled, normal_inverted=f.normal_inverted))
                    progress(i+1, len(self.frames))
                z.writestr('manifest.json', json.dumps(dict(version=3, variant=self.variant, source_collection_id=self.source_collection_id, material_channels={'metallic':'128..255 = metal; 0..127 = nonmetal', 'metallic_smoothness': '0..255 = 0..1; smoothness', 'thickness': '0..255 = 0..1; thickness, not transparency'}, name=self.name, normal_convention='tangent-x-right-y-up-z-viewer', runtime_ready=False, settings=self.settings(), frames=rows), ensure_ascii=False))
            os.replace(temp, path)
        finally:
            if os.path.exists(temp): os.unlink(temp)
