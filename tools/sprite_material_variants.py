"""Independent albedo variants sharing immutable source identities."""
import copy
import hashlib
import struct
from pathlib import Path
from PIL import Image


def runtime_hash(image):
    return hashlib.sha256(struct.pack('<II', *image.size) + image.tobytes()).hexdigest()


def collection_id(project):
    keys = [f.identity.get('source_runtime_sha256', runtime_hash(f.image)) for f in project.frames]
    return hashlib.sha256('\n'.join(keys).encode()).hexdigest()


def duplicate(project):
    if project.variant != 'shadowed':
        raise ValueError('Duplicate the original shadowed project')
    result = copy.deepcopy(project)
    result.variant = 'unshadowed'
    result.source_collection_id = collection_id(project)
    result.name += '--unshadowed'
    for f in result.frames:
        f.reference_image = f.image.copy()
        f.identity['source_runtime_sha256'] = runtime_hash(f.image)
    return result


def validate(project):
    if project.variant not in ('shadowed', 'unshadowed'):
        raise ValueError('Invalid albedo variant')
    if project.variant == 'unshadowed':
        for f in project.frames:
            original = getattr(f, 'reference_image', None)
            if original is None or original.size != f.image.size:
                raise ValueError('Missing or mismatched original albedo')
            if original.getchannel('A').tobytes() != f.image.getchannel('A').tobytes():
                raise ValueError('Albedo variants must preserve alpha coverage')
            if runtime_hash(original) != f.identity.get('source_runtime_sha256'):
                raise ValueError('Original source identity mismatch')
        if project.source_collection_id != collection_id(project):
            raise ValueError('Source collection identity mismatch')


def protect_target(project, path, runtime=False, recovery=False):
    """Never let a variant overwrite the other variant, including legacy files."""
    path = Path(path)
    if not path.exists():
        return
    if runtime:
        with path.open('rb') as f:
            other = 'unshadowed' if f.read(8) == b'MVRMAT06' else 'shadowed'
    else:
        import json, zipfile
        with zipfile.ZipFile(path) as z:
            # The rolling session snapshot intentionally follows the current
            # project, which can change variant. Never exempt ordinary saves.
            if recovery and path.name == 'sprite-editor-last-session.smproj' and 'editor-session.json' in z.namelist():
                return
            other = json.loads(z.read('project.json')).get('variant', 'shadowed')
    if other != project.variant:
        raise ValueError('Cannot overwrite the other albedo variant; choose a separate file')


def replace_albedo(project, index, image):
    if project.variant != 'unshadowed':
        raise ValueError('Albedo replacement requires an unshadowed project')
    image = image.convert('RGBA')
    f = project.frames[index]
    if image.size != f.image.size or image.getchannel('A').tobytes() != f.reference_image.getchannel('A').tobytes():
        raise ValueError('Replacement must preserve dimensions and alpha')
    from sprite_material import digest
    f.image = image.copy()
    f.identity['image_sha256'] = digest(f.image)


def copy_materials(target, source):
    validate(source); validate(target)
    if collection_id(target) != collection_id(source) or len(target.frames) != len(source.frames):
        raise ValueError('Projects do not share the same source collection')
    for dst, src in zip(target.frames, source.frames):
        if dst.image.size != src.image.size:
            raise ValueError('Frame dimensions differ')
    # Bake the evaluated maps, so changing albedo cannot change copied height/normal.
    layers = [(source.height(f).convert('RGBA'), source.normal(f)) for f in source.frames]
    for dst, src, (height, normal) in zip(target.frames, source.frames, layers):
        height.putalpha(255); normal.putalpha(255)
        dst.height_ink, dst.normal_ink = height, normal
        dst.normal_inverted = False  # copied normal is already evaluated
        for key in ('metallic', 'metallic_smoothness', 'thickness'):
            setattr(dst, key, getattr(src, key).copy())
    # PBR enablement is deliberately a separate user choice.
