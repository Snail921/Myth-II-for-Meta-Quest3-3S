"""Alpha-protected grayscale painting and auditable material edits."""
import datetime
import math
import numpy as np
from PIL import Image
from sprite_material import digest

CHANNELS = ('metallic_smoothness', 'metallic', 'thickness')


def record(project, operation, channel, before, **details):
    changes = []
    for i, old in before.items():
        new = getattr(project.frames[i], channel)
        count = int(np.count_nonzero(np.asarray(old) != np.asarray(new)))
        if count:
            changes.append(dict(index=i, name=project.frames[i].name, pixels=count,
                                before_sha256=digest(old), after_sha256=digest(new)))
    if changes:
        project.history.append(dict(id=len(project.history)+1,
            time=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            operation=operation, channel=channel, changes=changes, **details))
    return bool(changes)


def transform(project, indices, channel, operation, contrast=1, brightness=0, value=0):
    if channel not in CHANNELS: raise ValueError('Select a material layer')
    if operation not in ('copy_height', 'invert', 'contrast', 'brightness', 'fill'): raise ValueError('Unknown operation')
    if not math.isfinite(contrast) or not 0 <= contrast <= 4: raise ValueError('Contrast must be 0..4')
    if not math.isfinite(brightness) or not -1 <= brightness <= 1: raise ValueError('Brightness must be -1..1')
    if operation == 'fill' and (not math.isfinite(value) or not 0 <= value <= 1): raise ValueError('Fill value must be 0..1')
    before = {i: getattr(project.frames[i], channel).copy() for i in indices}
    for i in indices:
        f = project.frames[i]
        old = np.array(before[i])
        if operation == 'copy_height': new = np.asarray(project.height(f))
        elif operation == 'fill': new = np.full_like(old, round(value*255))
        elif operation == 'invert': new = 255-old
        elif operation == 'brightness': new = np.rint(np.clip(old.astype(float)+brightness*255,0,255)).astype('uint8')
        else: new = np.rint(np.clip((old.astype(float)/255-.5)*contrast+.5, 0, 1)*255).astype('uint8')
        visible = np.asarray(f.edit_mask()) > 0
        old[visible] = new[visible]
        setattr(f, channel, Image.fromarray(old))
    return before


class Stroke:
    """Maximum coverage per stroke makes Replace independent of motion event density."""
    def __init__(self, frame, channel, value, radius, softness, blend):
        if channel not in CHANNELS or blend not in ('Replace', 'Add', 'Sub', 'Screen'): raise ValueError('Invalid brush')
        self.frame, self.channel = frame, channel
        self.value, self.radius, self.softness, self.blend = value, radius, softness, blend
        self.before = getattr(frame, channel).copy()
        self.coverage = np.zeros((frame.image.height, frame.image.width), dtype=float)
        self.points = []
        self.painted_value = np.full_like(self.coverage, value)
        self.last_scales = None

    def point(self, x, y, radius_scale=1., value_scale=1.):
        previous = self.points[-1] if self.points else (x, y)
        self.points.append([x, y])
        old_radius, old_value = self.last_scales or (radius_scale, value_scale)
        self.last_scales = (radius_scale, value_scale)
        steps = max(1, math.ceil(math.dist(previous, (x,y))/max(.5, self.radius*min(old_radius,radius_scale)*.2)))
        for t in np.linspace(0, 1, steps+1):
            cx, cy = previous[0]+(x-previous[0])*t, previous[1]+(y-previous[1])*t
            radius=max(.5,self.radius*(old_radius+(radius_scale-old_radius)*t))
            value=self.value*(old_value+(value_scale-old_value)*t)
            x0=max(0,math.floor(cx-radius)); x1=min(self.frame.image.width,math.ceil(cx+radius)+1)
            y0=max(0,math.floor(cy-radius)); y1=min(self.frame.image.height,math.ceil(cy+radius)+1)
            yy,xx=np.mgrid[y0:y1,x0:x1]
            d=np.sqrt((xx-cx)**2+(yy-cy)**2)/radius
            a=np.clip((1-d)/max(self.softness,.0001),0,1)
            a=a*a*(3-2*a)
            values=self.painted_value[y0:y1,x0:x1]
            values[a>=self.coverage[y0:y1,x0:x1]]=value
            self.coverage[y0:y1,x0:x1]=np.maximum(self.coverage[y0:y1,x0:x1],a)
        base=np.asarray(self.before,dtype=float)/255
        a=self.coverage*(np.asarray(self.frame.edit_mask())>0)
        if self.blend=='Replace': result=base+(self.painted_value-base)*a
        elif self.blend=='Add': result=base+self.painted_value*a
        elif self.blend=='Sub': result=base-self.painted_value*a
        else: result=1-(1-base)*(1-self.painted_value*a)
        setattr(self.frame,self.channel,Image.fromarray(np.rint(np.clip(result,0,1)*255).astype('uint8')))
