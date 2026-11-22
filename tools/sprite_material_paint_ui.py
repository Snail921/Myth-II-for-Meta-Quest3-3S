"""Compact material tools window; editing remains on the main preview."""
import tkinter as tk
from tkinter import ttk, messagebox
from sprite_material_paint import CHANNELS, Stroke, transform, record


class MaterialPaintUI:
    def __init__(self, editor):
        self.e=editor; self.window=None; self.stroke=None; self.undo_state=None
        self.value=tk.DoubleVar(value=.5); self.radius=tk.IntVar(value=8)
        self.softness=tk.DoubleVar(value=.7); self.blend=tk.StringVar(value='Replace')
        self.tool=tk.StringVar(value='paint'); self.scope=tk.StringVar(value='current')
        self.contrast=tk.DoubleVar(value=1); self.brightness=tk.DoubleVar(value=0)
        self.brightness_edit=None; self.brightness_reset=False
        self.brightness.trace_add('write',self.preview_brightness)
        self.contrast_edit=None; self.contrast_reset=False
        self.contrast.trace_add('write',self.preview_contrast)
        self.radius_pressure=tk.BooleanVar(value=False)
        self.value_pressure=tk.BooleanVar(value=False)
        self.cursor=None
        self.line_anchor=None; self.line_context=None; self.shift_down=False
        for key in ('Shift_L','Shift_R'):
            editor.root.bind('<KeyPress-'+key+'>',lambda event:self.shift_preview(True),add='+')
            editor.root.bind('<KeyRelease-'+key+'>',lambda event:self.shift_preview(False),add='+')
        editor.root.bind('<FocusOut>',lambda event:self.shift_preview(False),add='+')
        editor.root.bind('<Control-z>',self.shortcut)
        editor.root.bind('<Control-Z>',self.shortcut)

    def open(self):
        if self.window and self.window.winfo_exists(): self.window.lift(); return
        e=self.e; w=self.window=tk.Toplevel(e.root)
        w.title(e.tr('材質ペイント','Material paint')); w.resizable(False,False)
        w.attributes('-topmost',True)
        w.bind('<Control-z>',self.shortcut);w.bind('<Control-Z>',self.shortcut)
        ttk.Label(w,text=e.tr('メイン画面のThickness / M&Sを選び、プレビュー上で描画。','Select Thickness / M&S, then paint on the main preview.')).pack(padx=10,pady=6)
        row=ttk.Frame(w); row.pack(fill='x',padx=8)
        for value,ja,en in [('paint','描画','Paint'),('pick','スポイト','Eyedropper')]:
            ttk.Radiobutton(row,text=e.tr(ja,en),variable=self.tool,value=value).pack(side='left')
        ttk.Combobox(row,textvariable=self.blend,values=['Replace','Add','Sub','Screen'],state='readonly',width=10).pack(side='right')
        row=ttk.Frame(w);row.pack(fill='x',padx=8)
        for variable,ja,en in [(self.radius_pressure,'筆圧で半径','Pen pressure: radius'),(self.value_pressure,'筆圧でカラー値','Pen pressure: value')]:
            ttk.Checkbutton(row,text=e.tr(ja,en),variable=variable).pack(side='left')
        for label,var,lo,hi in [(e.tr('値','Value'),self.value,0,1),(e.tr('半径 (px)','Radius (px)'),self.radius,1,64),(e.tr('柔らかさ','Softness'),self.softness,0,1)]:
            row=ttk.Frame(w);row.pack(fill='x',padx=8,pady=3)
            ttk.Label(row,text=label,width=14).pack(side='left')
            if var is self.radius:
                ttk.Scale(row,variable=var,from_=lo,to=hi,command=self.set_radius).pack(side='left',fill='x',expand=True)
                spin=ttk.Spinbox(row,textvariable=var,from_=lo,to=hi,increment=1,width=7,command=self.commit_radius)
                spin.pack(side='right');spin.bind('<FocusOut>',self.commit_radius);spin.bind('<Return>',self.commit_radius)
            else:
                ttk.Scale(row,variable=var,from_=lo,to=hi).pack(side='left',fill='x',expand=True)
                ttk.Spinbox(row,textvariable=var,from_=lo,to=hi,increment=.1,width=7).pack(side='right')
        row=ttk.Frame(w);row.pack(padx=8,pady=6)
        for i in range(11):
            c=round(i*25.5); color=f'#{c:02x}{c:02x}{c:02x}'
            tk.Button(row,text=f'{i/10:g}',width=3,bg=color,fg='white' if i<5 else 'black',command=lambda v=i/10:self.value.set(v)).pack(side='left')
        ttk.Separator(w).pack(fill='x',pady=6)
        row=ttk.Frame(w);row.pack(fill='x',padx=8)
        ttk.Label(row,text=e.tr('対象: current / all / 0-20,25','Scope: current / all / 0-20,25')).pack(side='left')
        ttk.Entry(row,textvariable=self.scope,width=18).pack(side='right')
        row=ttk.Frame(w);row.pack(fill='x',padx=8,pady=6)
        for label,op in [(e.tr('Heightをコピー','Copy Height'),'copy_height'),(e.tr('明度反転','Invert'),'invert'),(e.tr('塗りつぶし','Fill'),'fill')]:
            ttk.Button(row,text=label,command=lambda op=op:self.apply(op)).pack(side='left')
        row=ttk.Frame(w);row.pack(fill='x',padx=8,pady=6)
        ttk.Label(row,text=e.tr('コントラスト (1=無変更)','Contrast (1=unchanged)')).pack(side='left')
        self.contrast_scale=ttk.Scale(row,variable=self.contrast,from_=0,to=4)
        self.contrast_scale.pack(side='left',fill='x',expand=True)
        self.contrast_spin=ttk.Spinbox(row,textvariable=self.contrast,from_=0,to=4,increment=.1,width=7)
        self.contrast_spin.pack(side='left')
        for control in (self.contrast_scale,self.contrast_spin):
            control.bind('<ButtonRelease-1>',self.finish_contrast)
            control.bind('<KeyRelease>',self.contrast_key)
            control.bind('<FocusOut>',self.finish_contrast)
        row=ttk.Frame(w);row.pack(fill='x',padx=8,pady=6)
        ttk.Label(row,text=e.tr('明度 (0=無変更)','Brightness (0=unchanged)')).pack(side='left')
        self.brightness_scale=ttk.Scale(row,variable=self.brightness,from_=-1,to=1)
        self.brightness_scale.pack(side='left',fill='x',expand=True)
        self.brightness_spin=ttk.Spinbox(row,textvariable=self.brightness,from_=-1,to=1,increment=.05,width=7)
        self.brightness_spin.pack(side='left')
        for control in (self.brightness_scale,self.brightness_spin):
            control.bind('<ButtonRelease-1>',self.finish_brightness)
            control.bind('<KeyRelease>',self.brightness_key)
            control.bind('<FocusOut>',self.finish_brightness)
        w.protocol('WM_DELETE_WINDOW',self.close)
        ttk.Button(w,text=e.tr('材質編集を1回戻す (Ctrl+Z)','Undo last material edit (Ctrl+Z)'),command=self.undo).pack(pady=6)
        ttk.Label(w,text=e.tr('Sourceの透明ピクセルは常に保護。PBRチェックは別途有効にしてください。','Transparent source pixels are protected. Enable PBR separately.')).pack(padx=8,pady=6)

    def close(self):
        self.finish_brightness();self.finish_contrast()
        self.window.destroy();self.window=None

    def brightness_key(self,event):
        if event.keysym in ('Return','KP_Enter','Left','Right','Up','Down','Home','End'):
            self.finish_brightness();self.finish_contrast()

    def preview_brightness(self,*args):
        if self.brightness_reset:return
        e=self.e
        try:
            value=self.brightness.get()
            if not -1<=value<=1:return
            if self.brightness_edit is None:
                self.finish_contrast()
                if e.busy or not e.project.frames or e.view.get() not in CHANNELS:return
                from sprite_editor_commands import parse_frame_indices, revision
                indices=[e.index] if self.scope.get().strip().lower()=='current' else parse_frame_indices(self.scope.get(),len(e.project.frames))
                self.brightness_edit=dict(session=e.project_session,revision=revision(e.project),
                    indices=indices,channel=e.view.get(),value=value)
            self.brightness_edit['value']=value
            e.preview()
        except (ValueError,tk.TclError):return

    def preview_layer(self,frame,channel):
        return self.e.histogram.preview_layer(frame,channel,self.contrast_layer(frame,channel) if self.contrast_edit else self.brightness_layer(frame,channel))

    def brightness_layer(self,frame,channel):
        edit=self.brightness_edit;e=self.e
        if not edit or edit['session']!=e.project_session or channel!=edit['channel'] or not any(frame is e.project.frames[i] for i in edit['indices']):
            return getattr(frame,channel)
        from PIL import Image
        layer=getattr(frame,channel)
        shifted=layer.point([max(0,min(255,round(i+edit['value']*255))) for i in range(256)])
        return Image.composite(shifted,layer,frame.image.getchannel('A').point(lambda a:255 if a else 0))

    def finish_brightness(self,event=None):
        edit=self.brightness_edit
        if edit is None:return
        self.brightness_edit=None
        self.brightness_reset=True
        self.brightness.set(0)
        self.brightness_reset=False
        e=self.e
        from sprite_editor_commands import revision
        if e.busy or edit['session']!=e.project_session or edit['revision']!=revision(e.project):
            e.preview();return
        before=transform(e.project,edit['indices'],edit['channel'],'brightness',brightness=edit['value'])
        self.commit('brightness',edit['channel'],before,brightness=edit['value'])

    def contrast_key(self,event):
        if event.keysym in ('Return','KP_Enter','Left','Right','Up','Down','Home','End'):
            self.finish_contrast()

    def preview_contrast(self,*args):
        if self.contrast_reset:return
        e=self.e
        try:
            value=self.contrast.get()
            if not 0<=value<=4:return
            if self.contrast_edit is None:
                self.finish_brightness()
                if e.busy or not e.project.frames or e.view.get() not in CHANNELS:return
                from sprite_editor_commands import parse_frame_indices, revision
                indices=[e.index] if self.scope.get().strip().lower()=='current' else parse_frame_indices(self.scope.get(),len(e.project.frames))
                self.contrast_edit=dict(session=e.project_session,revision=revision(e.project),
                    indices=indices,channel=e.view.get(),value=value)
            self.contrast_edit['value']=value
            e.preview()
        except (ValueError,tk.TclError):return

    def contrast_layer(self,frame,channel):
        edit=self.contrast_edit;e=self.e
        if not edit or edit['session']!=e.project_session or channel!=edit['channel'] or not any(frame is e.project.frames[i] for i in edit['indices']):
            return getattr(frame,channel)
        from PIL import Image
        layer=getattr(frame,channel)
        shifted=layer.point([max(0,min(255,round(((i/255-.5)*edit['value']+.5)*255))) for i in range(256)])
        return Image.composite(shifted,layer,frame.image.getchannel('A').point(lambda a:255 if a else 0))

    def finish_contrast(self,event=None):
        edit=self.contrast_edit
        if edit is None:return
        self.contrast_edit=None
        self.contrast_reset=True
        self.contrast.set(1)
        self.contrast_reset=False
        e=self.e
        from sprite_editor_commands import revision
        if e.busy or edit['session']!=e.project_session or edit['revision']!=revision(e.project):
            e.preview();return
        before=transform(e.project,edit['indices'],edit['channel'],'contrast',contrast=edit['value'])
        self.commit('contrast',edit['channel'],before,contrast=edit['value'])

    def shortcut(self,event=None):
        self.finish_brightness();self.finish_contrast()
        self.undo()
        return 'break'

    def set_radius(self,value):
        self.radius.set(max(1,min(64,int(float(value)+.5))))
        self.draw_cursor()

    def commit_radius(self,event=None):
        try:self.set_radius(self.radius._tk.globalgetvar(self.radius._name))
        except (ValueError,tk.TclError):self.radius.set(8)

    def line_key(self):
        e=self.e
        if not e.project.frames:return None
        channel=e.view.get()
        return (e.project_session,e.index,id(e.project.frames[e.index]),channel,
                self.tool.get() if channel in CHANNELS else e.mode.get())

    def anchor(self):
        if self.line_context!=self.line_key():
            self.line_anchor=None;self.line_context=self.line_key()
        return self.line_anchor

    def set_anchor(self,x,y):
        self.line_context=self.line_key();self.line_anchor=(x,y)

    def shift_preview(self,pressed):
        self.shift_down=pressed
        self.draw_cursor()

    def draw_line_preview(self):
        e=self.e;c=e.canvas;c.delete('paint-line-preview')
        start=self.anchor()
        if not self.shift_down or self.cursor is None or start is None or e.busy:return
        channel=e.view.get()
        if (self.tool.get() if channel in CHANNELS else e.mode.get())=='pick':return
        x,y=self.cursor;ox,oy=e.offset;w,h=e.display_size
        if not (ox<=x<ox+w and oy<=y<oy+h):return
        sx,sy=ox+(start[0]+.5)*e.zoom,oy+(start[1]+.5)*e.zoom
        for color,width in [('black',3),('#00ffff',1)]:
            c.create_line(sx,sy,x,y,fill=color,width=width,dash=(5,3),tags='paint-line-preview')

    def motion(self,event):
        self.cursor=(event.x,event.y)
        self.shift_down=bool(event.state & 1)
        self.draw_cursor()

    def leave(self,event=None):
        self.cursor=None
        self.e.canvas.delete('material-brush-cursor')
        self.e.canvas.delete('paint-line-preview')

    def draw_cursor(self):
        e=self.e;c=e.canvas;c.delete('material-brush-cursor')
        self.draw_line_preview()
        if self.cursor is None or e.view.get() not in CHANNELS or self.tool.get()!='paint' or not e.project.frames:return
        x,y=self.cursor;ox,oy=e.offset;w,h=e.display_size
        if not (ox<=x<ox+w and oy<=y<oy+h):return
        try:r=self.radius.get()*e.zoom
        except tk.TclError:return
        for color,width in [('black',3),('white',1)]:
            c.create_oval(x-r,y-r,x+r,y+r,outline=color,width=width,tags='material-brush-cursor')

    def apply(self,op):
        e=self.e
        if not e.guard() or not e.project.frames:return
        self.finish_brightness();self.finish_contrast();self.finish()
        try:
            from sprite_editor_commands import parse_frame_indices
            indices=[e.index] if self.scope.get().strip().lower()=='current' else parse_frame_indices(self.scope.get(),len(e.project.frames))
            channel=e.view.get(); contrast=self.contrast.get()
            brightness=self.brightness.get()
            value=self.value.get() if op=='fill' else 0
            before=transform(e.project,indices,channel,op,contrast,brightness,value)
            self.commit(op,channel,before,value=value if op=='fill' else None,brightness=brightness if op=='brightness' else None,contrast=contrast if op=='contrast' else None,
                        height_settings=e.project.settings() if op=='copy_height' else None)
        except (ValueError,tk.TclError) as exc:messagebox.showerror('Material paint',str(exc),parent=self.window)

    def commit(self,op,channel,before,**details):
        e=self.e
        if record(e.project,op,channel,before,origin='user',**details):
            from sprite_editor_commands import revision
            self.undo_state=(e.project_session,revision(e.project),channel,before)
            e.dirty=True
        e.refresh()

    def paint(self,x,y,straight=False,pressure=1.):
        e=self.e; channel=e.view.get(); f=e.project.frames[e.index]
        if not f.image.getpixel((x,y))[3] and not (straight and self.anchor() is not None):return
        if self.tool.get()=='pick':
            self.value.set(getattr(f,channel).getpixel((x,y))/255);return
        if self.stroke is None:
            try:
                value=self.value.get();radius=self.radius.get();softness=self.softness.get()
                if not (0<=value<=1 and 1<=radius<=64 and 0<=softness<=1):raise ValueError('Invalid brush values')
                self.stroke=Stroke(f,channel,value,radius,softness,self.blend.get());self.stroke_index=e.index
                if straight and self.anchor() is not None:self.stroke.point(*self.anchor(), radius_scale=pressure if self.radius_pressure.get() else 1., value_scale=pressure if self.value_pressure.get() else 1.)
            except (ValueError,tk.TclError) as exc:messagebox.showerror('Material paint',str(exc));return
        self.stroke.point(x,y,pressure if self.radius_pressure.get() else 1.,pressure if self.value_pressure.get() else 1.);self.set_anchor(x,y);e.preview()

    def finish(self):
        s=self.stroke
        if s is None:return
        self.stroke=None
        self.commit('brush',s.channel,{self.stroke_index:s.before},value=s.value,radius=s.radius,
                    softness=s.softness,blend=s.blend,points=s.points)

    def undo(self):
        e=self.e
        if not e.guard():return
        self.finish()
        from sprite_editor_commands import revision
        u=self.undo_state
        if not u or u[:2]!=(e.project_session,revision(e.project)):
            messagebox.showinfo('Undo','No matching material edit to undo.');return
        _,_,channel,old=u
        before={i:getattr(e.project.frames[i],channel).copy() for i in old}
        for i,layer in old.items():setattr(e.project.frames[i],channel,layer)
        record(e.project,'undo',channel,before,origin='user')
        self.undo_state=None;e.dirty=True;e.refresh()
