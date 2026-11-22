"""Commands run on the UI thread; edits preserve all unrelated live work."""
import base64
import hashlib
import io
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image


def revision(project):
    h = hashlib.sha256(json.dumps(project.settings(), sort_keys=True).encode())
    for f in project.frames:
        h.update(json.dumps(f.identity, sort_keys=True).encode())
        h.update(b'PBR1' if f.pbr_enabled else b'PBR0')
        for key in ('image', 'mask', 'correction', 'correction_mask', 'metallic_smoothness', 'thickness'):
            layer = getattr(f, key)
            h.update(b'none' if layer is None else layer.tobytes())
    return h.hexdigest()


def response(status, **data):
    return status, 'application/json', json.dumps(data).encode()


def execute(bridge, route, body):
    e = bridge.editor
    p = e.project
    current = revision(p)
    if body.get('project_session') != e.project_session or body.get('revision') != current:
        return response(409, error='Project changed; fetch state and recompute the edit')
    try:
        if route in ('/save-project', '/load-project'):
            path = Path(body.get('path') or getattr(e, 'project_path', '') or '')
            if not path.is_absolute() or path.suffix.lower() != '.smproj':
                raise ValueError('An absolute .smproj path is required')
            if route == '/load-project' and e.dirty:
                return response(409, error='Save the live project before loading another project')
            if route == '/save-project' and path.exists() and body.get('overwrite') is not True:
                return response(409, error='Existing file requires overwrite=true')
            bridge.last_operation = dict(command=route, status='running', path=str(path))
            def work(progress):
                try:
                    if route == '/save-project':
                        p.save(path, progress)
                        return None
                    from sprite_material import Project
                    return Project.load(path, progress)
                except Exception as exc:
                    bridge.last_operation = dict(command=route, status='error', error=str(exc))
                    raise
            def done(loaded):
                if loaded is not None:
                    e.accept(loaded)
                e.project_path = str(path)
                e.dirty = False
                bridge.last_operation = dict(command=route, status='complete', path=str(path))
            e.task('Saving project' if route == '/save-project' else 'Loading project', work, done)
            return response(202, accepted=True)
        if route == '/material-undo':
            undo = getattr(bridge, 'material_undo', None)
            if not undo or undo[0] != e.project_session or undo[1] != current:
                return response(409, error='No matching edit to undo; subsequent edits are protected')
            _, _, channel, old, dirty = undo
            for index, layer in old.items():
                setattr(p.frames[index], channel, layer)
            e.dirty = dirty
            bridge.material_undo = None
            e.refresh()
            return response(200, revision=revision(p), undone=True)
        if route == '/pbr-set':
            enabled=body.get('enabled')
            if type(enabled) is not bool:raise ValueError('enabled must be boolean')
            indices=body.get('indices')
            if not isinstance(indices,list) or not indices or any(type(i) is not int or not 0<=i<len(p.frames) for i in indices) or len(set(indices))!=len(indices):
                raise ValueError('indices must be unique valid image indices')
            old={i:p.frames[i].pbr_enabled for i in indices if p.frames[i].pbr_enabled!=enabled}
            if body.get('dry_run') is not True and old:
                dirty=e.dirty
                for i in old:p.frames[i].pbr_enabled=enabled
                e.dirty=True
                bridge.material_undo=(e.project_session,revision(p),'pbr_enabled',old,dirty)
                e.refresh()
            return response(200,frames=len(old),enabled=enabled,dry_run=body.get('dry_run') is True,revision=revision(p))
        if route != '/material-edit':raise ValueError('Unknown command')
        channel = body.get('channel')
        if channel not in ('metallic_smoothness', 'thickness'):
            raise ValueError('Unknown material channel')
        value = body.get('value')
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or not 0 <= value <= 1:
            raise ValueError('value must be between 0 and 1')
        byte = round(value * 255)
        selections = {}
        # References guide the caller's visual/semantic analysis, never select pixels alone.
        if 'hint_index' in body:
            raise ValueError('Reference colors are hints, not segmentation; provide semantic masks')
        masks = body.get('masks')
        if not isinstance(masks, list) or not masks:
            raise ValueError('Provide semantic masks for the target parts')
        for row in masks:
            if not isinstance(row, dict):
                raise ValueError('Invalid mask entry')
            i = row['index']
            if type(i) is not int or not 0 <= i < len(p.frames) or i in selections:
                raise ValueError('Invalid or duplicate frame index')
            f = p.frames[i]
            if row.get('image_sha256') != f.identity['image_sha256']:
                raise ValueError('Source identity mismatch')
            with Image.open(io.BytesIO(base64.b64decode(row['png_base64'], validate=True))) as im:
                if im.format != 'PNG' or im.mode != 'L' or im.size != f.image.size:
                    raise ValueError('Mask must be a source-sized grayscale PNG')
                a = np.array(im)
            if not np.all((a == 0) | (a == 255)):
                raise ValueError('Mask must contain only 0 and 255')
            selections[i] = a == 255
        preview = None
        if 'preview_index' in body:
            i = body['preview_index']
            if type(i) is not int or i not in selections:
                raise ValueError('Preview index must be included in masks')
            f = p.frames[i]
            rgba = np.array(f.image)
            selected = selections[i] & (rgba[:, :, 3] > 0)
            rgba[selected, :3] = (rgba[selected, :3].astype(float) * .4 + np.array([255,40,70]) * .6).astype('uint8')
            out = io.BytesIO(); Image.fromarray(rgba).save(out, format='PNG')
            preview = base64.b64encode(out.getvalue()).decode()
        changes = {}
        pixels = 0
        for i, mask in selections.items():
            f = p.frames[i]
            a = np.array(getattr(f, channel))
            mask &= np.asarray(f.image)[:, :, 3] > 0
            mask &= a != byte
            if np.any(mask):
                pixels += int(mask.sum())
                a[mask] = byte
                changes[i] = Image.fromarray(a)
        if body.get('dry_run') is not True and changes:
            old = {i: getattr(p.frames[i], channel).copy() for i in changes}
            dirty = e.dirty
            for i, layer in changes.items():
                setattr(p.frames[i], channel, layer)
            from sprite_material_paint import record
            record(p,'material_edit',channel,old,origin='api',value=byte/255)
            e.dirty = True
            bridge.material_undo = (e.project_session, revision(p), channel, old, dirty)
            e.view.set(channel)
            e.refresh()
        return response(200, frames=len(changes), pixels=pixels, stored_value=byte / 255,
                        dry_run=body.get('dry_run') is True, preview_png_base64=preview, revision=revision(p))
    except (ValueError, TypeError, KeyError, OSError, IndexError) as exc:
        return response(400, error=str(exc))


def parse_frame_indices(text, count):
    if text.strip().lower()=='all':return list(range(count))
    result=set()
    for part in text.split(','):
        bounds=part.strip().split('-')
        if len(bounds) not in (1,2) or any(not b.strip().isdigit() for b in bounds):raise ValueError('Invalid image range')
        lo=int(bounds[0]);hi=int(bounds[-1])
        if not 0<=lo<=hi<count:raise ValueError('Image range is outside the collection')
        result.update(range(lo,hi+1))
    if not result:raise ValueError('No images selected')
    return sorted(result)
