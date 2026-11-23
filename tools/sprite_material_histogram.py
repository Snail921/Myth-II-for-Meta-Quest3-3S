"""Live histogram of the selected grayscale layer, excluding transparent pixels."""
import tkinter as tk
from tkinter import ttk, messagebox
from PIL import Image
import numpy as np


def levels(layer,source,low,high,mask=None):
    if not 0<=low<high<=255:raise ValueError('Black point must be below white point')
    table=[max(0,min(255,round((i-low)*255/(high-low)))) for i in range(256)]
    return Image.composite(layer.point(table),layer,mask if mask is not None else source.getchannel('A').point(lambda a:255 if a else 0))


def histogram(layer, source):
    visible = np.asarray(source.getchannel('A')) > 0
    return np.bincount(np.asarray(layer.convert('L'))[visible], minlength=256)


class MaterialHistogram:
    def __init__(self, editor):
        self.e=editor; self.window=None; self.counts=np.zeros(256,dtype=int)
        self.edit=None;self.low=0;self.high=255;self.active=None

    def open(self):
        if self.window and self.window.winfo_exists():self.window.lift();return
        e=self.e;w=self.window=tk.Toplevel(e.root)
        w.title(e.tr('ヒストグラム','Histogram'));w.geometry('560x320');w.minsize(360,240)
        w.attributes('-topmost',True);w.protocol('WM_DELETE_WINDOW',self.close)
        self.title=ttk.Label(w,anchor='w');self.title.pack(fill='x',padx=8,pady=4)
        self.canvas=tk.Canvas(w,bg='#242830',highlightthickness=0)
        self.canvas.pack(fill='both',expand=True,padx=8)
        self.summary=ttk.Label(w,anchor='w');self.summary.pack(fill='x',padx=8,pady=4)
        self.detail=ttk.Label(w,anchor='w');self.detail.pack(fill='x',padx=8,pady=4)
        self.canvas.bind('<Configure>',lambda event:self.update())
        self.canvas.bind('<Motion>',self.hover)
        self.canvas.bind('<Leave>',lambda event:self.detail.configure(text=''))
        self.canvas.bind('<Button-1>',self.press)
        self.canvas.bind('<B1-Motion>',self.drag)
        self.canvas.bind('<ButtonRelease-1>',self.release)
        w.bind('<Escape>',self.cancel)
        w.bind('<Control-z>',self.undo)
        w.bind('<Control-Z>',self.undo)
        self.update()

    def close(self):
        self.cancel()
        self.window.destroy();self.window=None

    def update(self):
        if not self.window or not self.window.winfo_exists():return
        e=self.e;c=self.canvas;c.delete('all');self.detail.configure(text='')
        mode=e.view.get();self.counts=np.zeros(256,dtype=int)
        if not e.project.frames or mode not in ('metallic_smoothness','metallic','thickness','height','mask'):
            self.title.configure(text=e.tr('Thickness / M&S / Height / Mask を選択してください。','Select Thickness / M&S / Height / Mask.'))
            self.summary.configure(text='');self.bounds=None;return
        f=e.project.frames[e.index]
        if mode in ('metallic_smoothness','metallic','thickness'):layer=e.material_paint.preview_layer(f,mode)
        elif mode=='height':layer=e.project.height(f)
        else:layer=f.mask
        self.counts=histogram(layer,f.image)
        preview=e.material_paint.brightness_edit or e.material_paint.contrast_edit
        label={'metallic_smoothness':'Smoothness','metallic':'Metallic','thickness':'Thickness','height':'Height','mask':'Mask'}[mode]
        live=bool(self.edit) or bool(preview and preview['session']==e.project_session and preview['channel']==mode and e.index in preview['indices'])
        self.title.configure(text=f'{e.index}: {f.name} — {label}'+(' (preview)' if live else ''))
        total=int(self.counts.sum());peak=int(self.counts.max())
        mean=float(np.dot(self.counts,np.arange(256))/total/255) if total else 0
        self.summary.configure(text=e.tr(f'対象 {total:,} px / 平均 {mean:.3f} / 透明部分は除外',f'{total:,} pixels / Mean {mean:.3f} / Transparent pixels excluded'))
        left,top,right,bottom=52,14,max(53,c.winfo_width()-14),max(15,c.winfo_height()-60)
        self.bounds=(left,top,right,bottom)
        for fraction in (0,.5,1):
            y=bottom-(bottom-top)*fraction
            c.create_line(left,y,right,y,fill='#414650')
            c.create_text(left-5,y,text=f'{round(peak*fraction):,}',anchor='e',fill='#dddddd')
        width=(right-left)/256
        for i,count in enumerate(self.counts):
            if count:
                c.create_rectangle(left+i*width,bottom-(bottom-top)*int(count)/max(1,peak),left+(i+1)*width,bottom,fill='#80b9e8',outline='')
        for fraction in (0,.25,.5,.75,1):
            c.create_text(left+(right-left)*fraction,bottom+15,text=f'{fraction:g}',fill='white')

        if mode in ('metallic_smoothness','metallic','thickness'):
            for value,color in [(self.low,'#202020'),(self.high,'#ffffff')]:
                x=left+(right-left)*value/255;y=bottom+32
                c.create_polygon(x,y,x-7,y+13,x+7,y+13,fill=color,outline='#dddddd',tags='level-handle')
            self.detail.configure(text=e.tr(f'黒点 {self.low/255:.3f} / 白点 {self.high/255:.3f} — ドラッグで補正',f'Black {self.low/255:.3f} / White {self.high/255:.3f} — drag to adjust'))

    def preview_layer(self,frame,channel,layer):
        edit=self.edit;e=self.e
        if not edit or edit['session']!=e.project_session or edit['channel']!=channel:return layer
        for i,base in edit['before'].items():
            if i<len(e.project.frames) and frame is e.project.frames[i]:
                return levels(base,frame.image,self.low,self.high,frame.edit_mask())
        return layer

    def press(self,event):
        e=self.e
        if not getattr(self,'bounds',None) or e.view.get() not in ('metallic_smoothness','metallic','thickness') or e.busy:return
        left,top,right,bottom=self.bounds
        if not bottom+24<=event.y<=bottom+52:return
        self.cancel()
        e.material_paint.finish_brightness();e.material_paint.finish_contrast();e.material_paint.finish()
        try:
            from sprite_editor_commands import revision
            indices=e.edit_indices()
            channel=e.view.get()
            self.edit=dict(session=e.project_session,revision=revision(e.project),channel=channel,
                           before={i:getattr(e.project.frames[i],channel).copy() for i in indices})
            self.active='low' if abs(event.x-left)<=abs(event.x-right) else 'high'
            self.drag(event)
        except ValueError as exc:messagebox.showerror('Levels',str(exc),parent=self.window)

    def drag(self,event):
        if not self.edit:return
        left,top,right,bottom=self.bounds
        value=round(max(0,min(1,(event.x-left)/(right-left)))*255)
        if self.active=='low':self.low=min(value,self.high-1)
        else:self.high=max(value,self.low+1)
        self.e.preview()

    def release(self,event=None):
        if not self.edit:return
        edit=self.edit;low,high=self.low,self.high
        self.edit=None;self.active=None;self.low=0;self.high=255
        e=self.e
        from sprite_editor_commands import revision
        if e.busy or edit['session']!=e.project_session or edit['revision']!=revision(e.project):
            e.preview();return
        for i,base in edit['before'].items():
            f=e.project.frames[i];setattr(f,edit['channel'],levels(base,f.image,low,high,f.edit_mask()))
        e.material_paint.commit('levels',edit['channel'],edit['before'],black_point=low/255,white_point=high/255)

    def cancel(self,event=None):
        self.edit=None;self.active=None;self.low=0;self.high=255
        self.e.preview()
        return 'break'

    def undo(self,event=None):
        if self.edit:return self.cancel()
        return self.e.material_paint.shortcut(event)

    def hover(self,event):
        if not getattr(self,'bounds',None):return
        left,top,right,bottom=self.bounds
        if not left<=event.x<=right:self.detail.configure(text='');return
        i=min(255,max(0,int((event.x-left)/(right-left)*256)))
        count=int(self.counts[i]);total=int(self.counts.sum())
        self.detail.configure(text=f'{i/255:.3f} ({i}/255): {count:,} px ({count/max(1,total)*100:.2f}%)')
