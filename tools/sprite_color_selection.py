"""Source color sampling and a transient, non-destructive paint selection."""
import math
import tkinter as tk
from tkinter import ttk, messagebox, colorchooser
from PIL import Image, ImageDraw, ImageChops
from sprite_material_paint import transform


def clean_small_components(mask, alpha, limit, *, remove=False):
    """Fill enclosed gaps or remove selected islands using 8-connectivity."""
    bounds = mask.getbbox()
    if bounds is None:
        return mask
    region = mask.crop(bounds)
    width, height = region.size
    pixels = bytearray(region.tobytes())
    visible = alpha.crop(bounds).tobytes()
    visited = bytearray(len(pixels))
    target = 255 if remove else 0
    for seed, selected in enumerate(pixels):
        if selected != target or visited[seed]:
            continue
        pending = [seed]
        visited[seed] = 1
        component = []
        enclosed = True
        while pending:
            pos = pending.pop()
            y, x = divmod(pos, width)
            if len(component) <= limit:
                component.append(pos)
            if not remove and (not visible[pos] or x in (0, width-1) or y in (0, height-1)):
                enclosed = False
            for ny in range(max(0, y-1), min(height, y+2)):
                for nx in range(max(0, x-1), min(width, x+2)):
                    neighbor = ny*width + nx
                    if pixels[neighbor] == target and not visited[neighbor]:
                        visited[neighbor] = 1
                        pending.append(neighbor)
        if enclosed and len(component) <= limit:
            for pos in component:
                pixels[pos] = 255 - target
    result = mask.copy()
    result.paste(Image.frombytes('L', region.size, bytes(pixels)), bounds)
    return result


class ColorSelection:
    def __init__(self,e,parent):
        self.e=e
        self.active=tk.BooleanVar(value=False)
        self.tolerance=tk.IntVar(value=20)
        self.component_limit=tk.IntVar(value=1)
        self.height=tk.IntVar(value=128)
        self.normal=(128,128,255)
        self.window=None
        self.reset()
        e.view.trace_add("write", self.view_changed)
        self.button=ttk.Button(parent,command=self.open)
        e.label(self.button,'色からマスクを作成','CreateMaskFromColor')
        self.button.pack(side='left')
        toggle=ttk.Checkbutton(parent,variable=self.active,command=self.toggle)
        toggle.pack(side='left');e.label(toggle,'選択マスク有効','Use selection')
        ink=ttk.Button(parent,command=self.settings)
        ink.pack(side='left');e.label(ink,'選択内ペイント','Selection paint')

    def reset(self):
        self.sample_view=self.e.view.get()
        self.applied=None
        self.colors=set()
        self.last=None
        self.sampling=False
        self.preview_job=None
        self.active.set(False)
        if self.window:self.window.destroy();self.window=None

    def view_changed(self, *_):
        if self.sampling and self.sample_view != self.e.view.get():
            self.sample_view = self.e.view.get()
            self.colors.clear()
            self.last = None

    def api_state(self):
        return dict(source_view=self.sample_view, reference_rgb=sorted(self.colors), tolerance=self.tolerance.get(),
                    scope=self.e.edit_scope.get(), sampling=self.sampling, enabled=self.active.get(),
                    applied=self.applied)

    def open(self):
        if not self.e.guard() or not self.e.project.frames:return
        self.sample_view=self.e.view.get();self.sampling=True;self.colors.clear();self.last=None
        self.settings();self.e.refresh()

    def settings(self):
        if self.window:self.window.lift();return
        w=self.window=tk.Toplevel(self.e.root);w.title(self.e.tr('色からマスクを作成','CreateMaskFromColor'))
        w.attributes('-topmost',True)
        w.protocol('WM_DELETE_WINDOW',self.close)
        tr=self.e.tr
        ttk.Label(w,text=tr('選択中の表示モードの右画像をドラッグして色を追加','Drag on the current view to sample colors')).pack()
        tk.Scale(w,from_=0,to=255,orient='horizontal',length=290,variable=self.tolerance,
                 label=tr('近似色許容','Color tolerance'),command=lambda _:self.queue_preview()).pack()
        ttk.Button(w,text=tr('マスクを適用','Apply selection'),command=self.apply).pack()
        holes=ttk.Frame(w);holes.pack()
        ttk.Button(holes,text=tr('小さなピクセルをマスク','Mask small pixels'),
                   command=lambda:self.apply(self.component_limit.get())).pack(side='left')
        ttk.Button(holes,text=tr('小さなピクセルを除去','Remove small pixels'),
                   command=lambda:self.apply(self.component_limit.get(), remove=True)).pack(side='left')
        ttk.Combobox(holes,textvariable=self.component_limit,values=(1,2,3,4),width=3,
                     state='readonly').pack(side='left')
        ttk.Label(holes,text=tr('px 以下','px or fewer')).pack(side='left')
        ttk.Button(w,text=tr('マスクをリセット','Reset Mask'),command=self.clear_mask).pack()
        tk.Scale(w,from_=0,to=255,variable=self.height,orient='horizontal',label=tr('高さ','Height')).pack()
        ttk.Button(w,text=tr('法線色を選択','Choose normal RGB'),command=self.choose_normal).pack()
        ttk.Button(w,text=tr('塗りつぶし','Fill'),command=self.fill).pack()

    def close(self):
        self.sampling=False;self.window.destroy();self.window=None;self.e.preview()

    def queue_preview(self):
        if self.preview_job is None:
            self.preview_job=self.e.root.after_idle(self.update_preview)

    def update_preview(self):
        self.preview_job=None
        self.e.preview()

    def sample(self,f,x,y,first):
        if first or self.last is None:self.last=(x,y)
        u,v=self.last
        steps=max(1,abs(x-u),abs(y-v))
        for i in range(steps+1):
            point=(round(u+(x-u)*i/steps),round(v+(y-v)*i/steps))
            if f.image.getpixel(point)[3]:self.colors.add(self.e.hover_image.getpixel(point)[:3])
        self.last=(x,y);self.queue_preview()

    def overlay(self,f,im):
        if self.sampling:
            mask=self.e.project.color_mask(f,self.colors,self.tolerance.get(),im)
        elif self.active.get():mask=f.selection
        else:return im
        if mask is None:return im
        rgb=Image.composite(Image.blend(im.convert('RGB'),Image.new('RGB',im.size,'cyan'),.45),im.convert('RGB'),mask)
        rgb.putalpha(im.getchannel('A'));return rgb

    def apply(self,component_limit=None, *, remove=False):
        if not self.e.guard():return
        existing=component_limit is not None and not self.sampling
        if existing and self.applied is None:return
        if not existing and not self.colors:return
        try:indices=self.e.edit_indices()
        except ValueError as err:messagebox.showerror('CreateMaskFromColor',str(err));return
        colors=tuple(self.colors);tol=self.tolerance.get();p=self.e.project
        source_view=self.sample_view
        def build(progress):
            masks={}
            for n,i in enumerate(indices):
                frame=p.frames[i]
                mask=frame.selection if existing else p.color_mask(frame,colors,tol,p.view_image(frame,source_view))
                if mask is not None and component_limit is not None:
                    mask=clean_small_components(mask,frame.image.getchannel('A'),component_limit,remove=remove)
                masks[i]=mask
                progress(n+1,len(indices))
            return masks
        def done(masks):
            if not existing:
                self.applied=dict(source_view=source_view,reference_rgb=sorted(colors),tolerance=tol,indices=indices)
                for f in p.frames:f.selection=None
            for i,mask in masks.items():p.frames[i].selection=mask
            self.active.set(True);self.sampling=False;self.toggle()
        self.e.task('CreateMaskFromColor',build,done)

    def toggle(self):
        for f in self.e.project.frames:f.selection_active=self.active.get()
        self.e.refresh()

    def clear_mask(self):
        if not self.e.guard():return
        self.colors.clear()
        self.applied=None
        self.last=None
        self.active.set(False)
        for frame in self.e.project.frames:
            frame.selection=None
            frame.selection_active=False
        self.sample_view=self.e.view.get()
        self.sampling=True
        self.e.refresh()

    def choose_normal(self):
        rgb,_=colorchooser.askcolor('#%02x%02x%02x'%self.normal,parent=self.window)
        if rgb:
            x,y,z=[v/127.5-1 for v in rgb];z=max(0,z)
            length=math.sqrt(x*x+y*y+z*z)
            if length:self.normal=tuple(round((v/length+1)*127.5) for v in (x,y,z))

    def geometry(self,f,mode,mask):
        if not mask.getbbox():return
        key=mode+'_ink';ink=getattr(f,key)
        if ink is None:ink=Image.new('RGBA',f.image.size);setattr(f,key,ink)
        color=(self.height.get(),)*3 if mode=='height' else self.normal
        ink.paste((*color,255),(0,0,*ink.size),mask);self.e.dirty=True

    def brush(self,f,mode,x,y,first):
        mask=Image.new('L',f.image.size);draw=ImageDraw.Draw(mask);r=int(self.e.params['brush'].get())
        if not first and self.last:draw.line([self.last,(x,y)],fill=255,width=max(1,2*r))
        draw.ellipse((x-r,y-r,x+r,y+r),fill=255);self.last=(x,y)
        self.geometry(f,mode,ImageChops.multiply(mask,f.edit_mask()));self.e.preview()

    def fill(self):
        e=self.e
        if not e.guard() or not e.project.frames:return
        try:indices=self.e.edit_indices()
        except ValueError as err:messagebox.showerror('Fill',str(err));return
        mode=e.view.get()
        if mode in ('height','normal'):
            for i in indices:
                f=e.project.frames[i];self.geometry(f,mode,f.edit_mask())
            e.refresh()
        elif mode in ('metallic','metallic_smoothness','thickness'):
            value=e.material_paint.value.get()
            before=transform(e.project,indices,mode,'fill',value=value)
            e.material_paint.commit('fill',mode,before,value=value)
