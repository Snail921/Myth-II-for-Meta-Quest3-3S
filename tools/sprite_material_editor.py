"""First-pass batch height/normal editor. Launch standalone or from Plugin Manager."""
import json
import os
import queue
import uuid
import zipfile
import threading
from pathlib import Path

local = Path(__file__).parent/'runtime/plugin-manager-tcl'
if (local/'tcl8.6/init.tcl').is_file():
    os.environ['TCL_LIBRARY'] = str(local/'tcl8.6')
    os.environ['TK_LIBRARY'] = str(local/'tk8.6')
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog
from PIL import Image, ImageTk, ImageDraw
from sprite_material import Project
from sprite_editor_files import DialogPaths
from sprite_material_hints import MaterialHints


class Editor:
    def __init__(self, root):
        self.paths = DialogPaths()
        self.project_session = uuid.uuid4().hex
        self.bridge = None
        self.material_ui = MaterialHints(self)
        self.collection_browsers = {}; self.last_collection_browser = None
        self.root = root
        from sprite_material_paint_ui import MaterialPaintUI
        self.material_paint = MaterialPaintUI(self)
        from sprite_material_histogram import MaterialHistogram
        self.histogram = MaterialHistogram(self)
        self.project = Project(); self.index = 0; self.page = 0
        self.busy = False; self.dirty = False; self.events = queue.Queue(); self.photos = []; self.last_point = None; self.refresh_job = None
        root.title('Myth II VR — Sprite Material Editor'); root.geometry('1280x850')
        self.widgets = []; self.lang = tk.StringVar(value='English')
        top = ttk.Frame(root, padding=6); top.pack(fill='x')
        for ja, en, fn in [('画像フォルダ','Image folder',self.folder), ('ユニット選択','Select unit',self.scene), ('Collectionを開く','Open collection',self.collection), ('コレクションフォルダ','Collection folder',self.browse_collections), ('プロジェクト読込','Load project',self.load), ('保存','Save',self.save), ('全画像を書出し','Export all',self.export), ('Quest用書出し','Export for Quest',self.export_runtime)]:
            b = ttk.Button(top, command=fn); b.pack(side='left', padx=3); self.label(b,ja,en)
        language = ttk.Combobox(top, values=['日本語','English'], textvariable=self.lang, state='readonly', width=9); language.pack(side='right'); language.bind('<<ComboboxSelected>>',lambda e:self.translate())
        tabs = ttk.Frame(root, padding=6); tabs.pack(fill='x')
        for ja,en,fn in [('材質の代表色','Material colors',self.material_ui.open),('直接連携','Direct connection',self.toggle_bridge)]:
            b=ttk.Button(tabs,command=fn);b.pack(side='right',padx=3);self.label(b,ja,en)
        self.root.bind('<Escape>',lambda e:setattr(self.material_ui,'pick_target',None))
        self.view = tk.StringVar(value='source')
        for value,ja,en in [('source','元画像','Source'),('mask','マスク','Mask'),('height','グレースケール','Height'),('normal','ノーマル','Normal'),('metallic_smoothness','Metallic＆Smoothness','Metallic & Smoothness'),('thickness','Thickness','Thickness')]:
            b=ttk.Radiobutton(tabs,variable=self.view,value=value,command=self.refresh);b.pack(side='left',padx=6);self.label(b,ja,en)
        self.correction_on=tk.BooleanVar(value=True)
        correction=ttk.Checkbutton(tabs,variable=self.correction_on,command=self.toggle_correction);correction.pack(side='left',padx=6);self.label(correction,'形状補正','Shape correction')
        self.back=ttk.Button(tabs,text='◀',command=lambda:self.turn(-1));self.back.pack(side='left',padx=10)
        self.pages=ttk.Label(tabs);self.pages.pack(side='left')
        ttk.Button(tabs,text='▶',command=lambda:self.turn(1)).pack(side='left')
        panes=ttk.Panedwindow(root,orient='horizontal');panes.pack(fill='both',expand=True,padx=6)
        self.gallery=tk.Canvas(panes,bg='#242830',highlightthickness=0,width=700);panes.add(self.gallery,weight=3)
        self.gallery_hover=None;self.pixel_hover=None;self.gallery_regions=[]
        self.gallery.bind('<Button-1>',self.select)
        self.gallery.bind('<Motion>',self.hover_gallery)
        self.gallery.bind('<Leave>',lambda event:self.clear_hover('gallery'))
        right=ttk.Frame(panes);panes.add(right,weight=2)
        self.canvas=tk.Canvas(right,bg='#343840',highlightthickness=0,width=480,height=390);self.canvas.pack(fill='both',expand=True)
        from sprite_pen_input import CanvasPen
        self.pen_input = CanvasPen(self.canvas)
        self.canvas.bind('<Button-1>',self.paint);self.canvas.bind('<B1-Motion>',self.paint);self.canvas.bind('<ButtonRelease-1>',self.end_paint);self.canvas.bind('<Configure>',lambda e:self.preview())
        self.canvas.bind('<Motion>',self.material_paint.motion)
        self.canvas.bind('<Leave>',self.material_paint.leave)
        self.canvas.bind('<Motion>',self.hover_pixel,add='+')
        self.canvas.bind('<B1-Motion>',self.hover_pixel,add='+')
        self.canvas.bind('<Leave>',lambda event:self.clear_hover('pixel'),add='+')
        self.caption=ttk.Label(right,wraplength=480);self.caption.pack(fill='x')
        pbr=ttk.Frame(right,padding=6);pbr.pack(fill='x')
        self.gallery_checks=[]
        ttk.Style(root).configure('GalleryPbr.TCheckbutton',background='#242830')
        b=ttk.Button(pbr,command=self.batch_pbr);b.pack(side='left',padx=4);self.label(b,'PBR一括設定','Batch PBR')
        b=ttk.Button(pbr,command=self.material_paint.open);b.pack(side='right');self.label(b,'材質ペイント','Material paint')
        b=ttk.Button(pbr,command=self.histogram.open);b.pack(side='right');self.label(b,'ヒストグラム','Histogram')
        self.pbr_summary=ttk.Label(pbr);self.pbr_summary.pack(side='left')
        edit=ttk.Frame(right,padding=6);edit.pack(fill='x')
        self.mode=tk.StringVar(value='pick')
        for value,ja,en in [('pick','スポイト（元画像）','Pick source color'),('paint','マスク追加','Paint mask'),('erase','マスク除外','Erase mask')]:
            b=ttk.Radiobutton(edit,variable=self.mode,value=value);b.pack(side='left');self.label(b,ja,en)
        settings=ttk.Frame(right,padding=6);settings.pack(fill='x');settings.columnconfigure(1,weight=1)
        self.params={}
        for row,(key,ja,en,initial,lo,hi) in enumerate([('tolerance','色の許容差','Color tolerance',30,0,255),('height_value','塗りつぶし値','Height fill',128,0,255),('strength','凹凸強度','Normal strength',2,0,20),('blur','ぼかし','Blur',0,0,5),('brush','ブラシ半径(px)','Brush radius (px)',8,1,64)]):
            label=ttk.Label(settings);label.grid(row=row,column=0,sticky='w');self.label(label,ja,en)
            var=tk.DoubleVar(value=initial);self.params[key]=var
            ttk.Scale(settings,from_=lo,to=hi,variable=var,command=lambda _,k=key:self.parameter(k)).grid(row=row,column=1,sticky='ew')
            if key=='strength':
                self.strength_text=tk.StringVar(value=str(initial))
                self.strength_entry=ttk.Spinbox(settings,from_=lo,to=hi,increment=1,textvariable=self.strength_text,width=8,command=self.commit_strength)
                self.strength_entry.grid(row=row,column=2)
                self.strength_entry.bind('<Return>',self.commit_strength)
                self.strength_entry.bind('<FocusOut>',self.commit_strength)
            else:ttk.Label(settings,textvariable=var,width=8).grid(row=row,column=2)
        self.colors=ttk.Label(right,wraplength=480);self.colors.pack(fill='x',padx=6)
        actions=ttk.Frame(right);actions.pack(fill='x',pady=6)
        for ja,en,fn in [('指定色を消去','Clear colors',self.clear_colors),('全画像のマスクを更新','Apply mask to all',self.masks),('このマスクを消去','Clear this mask',self.clear_mask)]:
            b=ttk.Button(actions,command=fn);b.pack(side='left');self.label(b,ja,en)
        note=ttk.Label(right,wraplength=480);note.pack(fill='x',padx=6,pady=6)
        self.label(note,'色指定の更新は手修正を置換します。マスク表示で直接編集できます。書出しはハイト・ノーマル・マスク・対応表のZIPです。Quest用書出しで法線・M&S・Thicknessを含むVR向け材質パックを作成できます。','Applying colors replaces manual masks. Paint in Mask view. Export contains heights, normals, masks and mapping. Export for Quest includes normals, M&S and Thickness.')
        self.progress=ttk.Progressbar(root,mode='determinate');self.progress.pack(fill='x',padx=6,pady=4)
        self.status=tk.StringVar();ttk.Label(root,textvariable=self.status).pack(fill='x',padx=6,pady=4)
        root.protocol('WM_DELETE_WINDOW',self.close);self.poll_job=root.after(80,self.poll);self.refresh()

    def label(self,w,ja,en):self.widgets.append((w,ja,en));w.configure(text=ja if self.lang.get()=='日本語' else en)
    def tr(self,ja,en):return ja if self.lang.get()=='日本語' else en
    def translate(self):
        for w,ja,en in self.widgets:w.configure(text=self.tr(ja,en))
        self.refresh()
    def guard(self):
        self.material_paint.finish()
        if self.busy:messagebox.showinfo('Working',self.tr('処理中です。進捗表示をご確認ください。','Working; please check the progress below.'));return False
        return True
    def replace_ok(self):return self.guard() and (not self.dirty or messagebox.askyesno('Unsaved changes',self.tr('未保存の編集を破棄しますか？','Discard unsaved changes?')))
    def task(self,title,fn,done):
        if not self.guard():return
        self.busy=True;self.status.set(title);self.progress.configure(value=0)
        def progress(i,n):self.events.put(('progress',(title,i,n)))
        def run():
            try:self.events.put(('done',(done,fn(progress))))
            except Exception as exc:self.events.put(('error',str(exc)))
        threading.Thread(target=run,daemon=True).start()
    def poll(self):
        try:
            while True:
                kind,data=self.events.get_nowait()
                if kind=='progress':
                    title,i,n=data;self.status.set(f'{title} ({i:04d}/{n:04d})');self.progress.configure(maximum=max(n,1),value=i)
                elif kind=='done':
                    self.busy=False;fn,result=data;fn(result);self.status.set(self.tr('完了','Complete'));self.refresh()
                else:self.busy=False;self.status.set(data);messagebox.showerror('Error',data)
        except queue.Empty:pass
        self.poll_job=self.root.after(80,self.poll)
    def accept(self,p):
        self.material_ui.close()
        self.project_session = uuid.uuid4().hex
        self.project=p;self.index=self.page=0;self.dirty=False;self.project_path=None
        if any(f.correction is not None for f in p.frames):self.view.set('height')
        self.sync()
    def accept_import(self,p):
        # Keep working slider values across source collections, not saved projects.
        for key in self.params:
            if key!='brush':setattr(p,key,getattr(self.project,key))
        self.accept(p)
    def toggle_correction(self):
        if self.busy:self.correction_on.set(self.project.correction_enabled);return
        self.project.correction_enabled=self.correction_on.get();self.dirty=True;self.refresh()
    def sync(self):
        self.correction_on.set(self.project.correction_enabled)
        for k,v in self.params.items():
            if k!='brush':v.set(getattr(self.project,k))
        self.strength_text.set(f'{self.project.strength:g}')
    def dialog(self, operation, kind, **options):
        options.update(parent=self.root, initialdir=self.paths.initial(operation))
        selected=getattr(filedialog,kind)(**options)
        if selected:
            try:self.paths.remember(operation,selected,kind=='askdirectory')
            except OSError as exc:messagebox.showerror('Settings',str(exc),parent=self.root)
        return selected
    def open_collection_browser(self):
        if not self.guard():return
        if self.last_collection_browser is None:self.browse_collections()
        else:self.last_collection_browser.reopen()
    def browse_collections(self):
        if not self.guard():return
        folder=self.dialog('collection_browser','askdirectory')
        if folder:
            try:
                from sprite_collection_browser import CollectionBrowser
                key=str(Path(folder).resolve())
                browser=self.collection_browsers.get(key)
                if browser is None:
                    browser=CollectionBrowser(self,key)
                    self.collection_browsers[key]=browser
                else:browser.reopen()
                self.last_collection_browser=browser
            except Exception as exc:messagebox.showerror('Error',str(exc),parent=self.root)
    def folder(self):
        if not self.replace_ok():return
        p=self.dialog('image_folder','askdirectory')
        if p:self.task(self.tr('画像読込','Loading images'),lambda cb:Project.folder(p,cb),self.accept_import)
    def collection(self,path=None,tag=None):
        if path is None:
            self.open_collection_browser();return
        if not self.replace_ok():return
        p=path
        if p:self.task(self.tr('スプライト読込','Loading sprites'),lambda cb:Project.collection(p,tag,cb),self.accept_import)
    def scene(self):
        if not self.guard():return
        p=self.dialog('scene_open','askopenfilename',title='Select converter scene/scene.json',filetypes=[('Scene','scene.json')])
        if not p:return
        try:assets=json.loads(Path(p).read_text(encoding='utf8'))['assets']
        except Exception as e:messagebox.showerror('Error',str(e));return
        dialog=tk.Toplevel(self.root);dialog.title(self.tr('素材を選択','Select asset'));dialog.geometry('600x450')
        box=tk.Listbox(dialog);box.pack(fill='both',expand=True)
        for a in assets:box.insert('end',f"{a.get('name',a['file'])}   {a.get('tag_id_hex','')}")
        def choose():
            if not box.curselection():return
            a=assets[box.curselection()[0]];dialog.destroy();self.collection(str(Path(p).parent/a['file']),a.get('tag_id_hex'))
        ttk.Button(dialog,text='OK',command=choose).pack();box.bind('<Double-1>',lambda e:choose())
    def load(self):
        if not self.replace_ok():return
        p=self.dialog('project_open','askopenfilename',filetypes=[('Sprite material project','*.smproj')])
        if p:self.task(self.tr('プロジェクト読込','Loading project'),lambda cb:Project.load(p,cb),lambda result:self.loaded_project(result,p))
    def save(self):
        if not self.guard() or not self.project.frames:return
        p=self.dialog('project_save','asksaveasfilename',defaultextension='.smproj',initialfile=self.project.name+'.smproj')
        if p:self.task(self.tr('保存','Saving'),lambda cb:self.project.save(p,cb),lambda _:self.saved_project(p))
    def loaded_project(self,project,path):
        self.accept(project);self.project_path=str(Path(path).resolve())
    def saved_project(self,path):
        self.project_path=str(Path(path).resolve());self.dirty=False
    def export(self):
        if not self.guard() or not self.project.frames:return
        p=self.dialog('export_images','asksaveasfilename',defaultextension='.zip',initialfile=self.project.name+'-materials.zip')
        if p:self.task(self.tr('全画像を生成・書出し','Generating and exporting all'),lambda cb:self.project.export(p,cb),lambda _:None)
    def export_runtime(self):
        if not self.guard() or not self.project.frames:return
        p=self.dialog('export_quest','asksaveasfilename',initialfile=self.project.name+'.mvrnm',defaultextension='.mvrnm',filetypes=[('Quest sprite materials','*.mvrnm')])
        if p:self.task(self.tr('Quest用材質生成','Generating Quest materials'),lambda cb:self.project.export_runtime(p,cb),lambda _:None)
    def masks(self):
        if not self.guard() or not self.project.frames:return
        if not messagebox.askyesno('Masks',self.tr('全画像のマスクを色指定から再作成します。手修正は置換されます。続行しますか？','Rebuild all masks from colors? This replaces manual edits.')):return
        self.task(self.tr('マスク生成','Generating masks'),lambda cb:self.project.apply_masks(cb),lambda _:setattr(self,'dirty',True))
    def clear_colors(self):
        if self.guard():self.project.colors=[];self.dirty=True;self.refresh()
    def clear_mask(self):
        if self.guard() and self.project.frames:self.project.frames[self.index].mask.paste(0,(0,0,*self.project.frames[self.index].image.size));self.dirty=True;self.refresh()
    def commit_strength(self,event=None):
        if self.busy:
            self.strength_text.set(f'{self.project.strength:g}');return
        try:
            value=int(self.strength_text.get().strip())
            if not 0<=value<=20:raise ValueError()
        except ValueError:
            self.strength_text.set(f'{self.project.strength:g}');self.root.bell();return
        if value==self.project.strength:
            self.strength_text.set(str(value));return
        self.params['strength'].set(value);self.parameter('strength')
    def parameter(self,key):
        if self.busy:self.sync();return
        if key=='strength':
            value=max(0,min(20,int(self.params[key].get()+.5)))
            self.params[key].set(value);self.strength_text.set(str(value))
        if key!='brush':
            value=self.params[key].get();setattr(self.project,key,int(round(value)) if key in ('tolerance','height_value') else round(value,2));self.dirty=True
            self.preview()
            if self.refresh_job:self.root.after_cancel(self.refresh_job)
            self.refresh_job=self.root.after(250,self.delayed_refresh)
    def delayed_refresh(self):
        self.refresh_job=None
        if not self.busy:self.refresh()
    def end_paint(self,event):
        self.material_paint.finish()
        self.last_point=None
        if not self.busy:self.refresh()
    def image(self,f):
        mode=self.view.get()
        if mode=='source':return f.image
        if mode in ('metallic_smoothness','thickness'):
            g=self.material_paint.preview_layer(f,mode)
            return Image.merge('RGBA',(g,g,g,f.image.getchannel('A')))
        if mode=='normal':return self.project.normal(f)
        if mode=='height':
            g=self.project.height(f);return Image.merge('RGBA',(g,g,g,f.image.getchannel('A')))
        result=f.image.copy();overlay=Image.new('RGBA',f.image.size,(255,40,70,0));overlay.putalpha(f.mask.point(lambda x:x//2));return Image.alpha_composite(result,overlay)
    def toggle_pbr(self,index,enabled):
        if not self.guard():
            self.refresh();return
        if not 0<=index<len(self.project.frames):return
        f=self.project.frames[index]
        if f.pbr_enabled!=enabled:f.pbr_enabled=enabled;self.dirty=True
        self.refresh()
    def batch_pbr(self):
        if not self.guard() or not self.project.frames:return
        text=simpledialog.askstring('Batch PBR',self.tr('対象の画像番号（0始まり、例: 0-20,25）。全画像は all。','Image indices (zero-based, e.g. 0-20,25). Use all for every image.'),parent=self.root)
        if text is None:return
        try:
            from sprite_editor_commands import parse_frame_indices
            indices=parse_frame_indices(text,len(self.project.frames))
        except ValueError as exc:messagebox.showerror('Batch PBR',str(exc));return
        enabled=messagebox.askyesnocancel('Batch PBR',self.tr('指定画像の疑似PBRを有効にしますか？ はい=ON、いいえ=OFF','Enable PBR for these images? Yes=ON, No=OFF'),parent=self.root)
        if enabled is None:return
        for i in indices:
            if self.project.frames[i].pbr_enabled!=enabled:self.project.frames[i].pbr_enabled=enabled;self.dirty=True
        self.refresh()
    def refresh(self):
        self.material_paint.finish()
        for check,var in self.gallery_checks:check.destroy()
        self.gallery_checks=[]
        self.gallery_regions=[]
        self.photos=[];self.gallery.delete('all');n=len(self.project.frames);pages=max(1,(n+47)//48);self.page=min(self.page,pages-1)
        self.pages.configure(text=f'{self.page+1}/{pages} · {n} images')
        if self.view.get() in ('metallic_smoothness','thickness'):
            self.status.set(self.tr('材質レイヤー：黒=0、白=1（初期値：Metallic＆Smoothness=0、Thickness=1）。Quest用材質パックに含まれます。','Material layer: black=0, white=1 (defaults: Metallic & Smoothness=0, Thickness=1). Included in Quest material output.'))
        self.pbr_summary.configure(text=f'PBR {sum(f.pbr_enabled for f in self.project.frames)}/{n}')
        self.colors.configure(text='RGB: '+', '.join(str(tuple(c)) for c in self.project.colors))
        for offset,f in enumerate(self.project.frames[self.page*48:self.page*48+48]):
            im=self.image(f).copy();im.thumbnail((90,83));photo=ImageTk.PhotoImage(im);self.photos.append(photo)
            x=(offset%6)*110+55;y=(offset//6)*83+42
            item=self.gallery.create_image(x,y-4,image=photo)
            self.gallery_regions.append((self.gallery.bbox(item),self.page*48+offset))
            self.gallery.create_text(x,y+33,text=f'{self.page*48+offset}: '+f.name[:12]+(' [PBR]' if f.pbr_enabled else ''),fill='white',font=('Segoe UI',8))
            if self.page*48+offset==self.index:self.gallery.create_rectangle(x-53,y-41,x+53,y+40,outline='#459fff',width=2)
            if self.view.get()=='source':
                index=self.page*48+offset;var=tk.BooleanVar(value=f.pbr_enabled)
                check=ttk.Checkbutton(self.gallery,variable=var,style='GalleryPbr.TCheckbutton',takefocus=True,
                    command=lambda i=index,v=var:self.toggle_pbr(i,v.get()))
                if self.busy:check.state(['disabled'])
                self.gallery_checks.append((check,var))
                self.gallery.create_window(x+49,y-38,window=check,anchor='ne')
        self.show_gallery_hover()
        self.preview()
    def preview(self):
        self.histogram.update()
        if not self.project.frames:
            self.canvas.delete('hover-info');return
        f=self.project.frames[self.index];im=self.image(f);self.hover_image=im;w=max(10,self.canvas.winfo_width());h=max(10,self.canvas.winfo_height());self.zoom=min((w-12)/im.width,(h-12)/im.height)
        self.display_size=(max(1,int(im.width*self.zoom)),max(1,int(im.height*self.zoom)))
        im=im.resize(self.display_size,Image.Resampling.NEAREST);self.offset=((w-im.width)//2,(h-im.height)//2)
        self.photo=ImageTk.PhotoImage(im);self.canvas.delete('all');self.canvas.create_image(*self.offset,image=self.photo,anchor='nw')
        self.caption.configure(text=f'{f.name}  {f.image.width}×{f.image.height}  · {len(f.identity.get("uses",[]))} frame/direction references'+(' · '+self.tr('形状補正あり','Shape correction') if f.correction is not None else ''))
        self.material_paint.draw_cursor()
        self.show_pixel_hover()
    def hover_label(self,canvas,x,y,text):
        canvas.delete('hover-info')
        label=canvas.create_text(x+14,y-14,text=text,anchor='sw',fill='white',font=('Segoe UI',10),tags='hover-info')
        box=canvas.bbox(label)
        # Keep the readout visible at the top/right edges.
        dx=min(0,canvas.winfo_width()-6-box[2]);dy=max(0,6-box[1])
        canvas.move(label,dx,dy);box=canvas.bbox(label)
        background=canvas.create_rectangle(box[0]-4,box[1]-2,box[2]+4,box[3]+2,fill='#16191e',outline='#8d949e',tags='hover-info')
        canvas.tag_lower(background,label)

    def clear_hover(self,which):
        if which=='gallery':self.gallery_hover=None;self.gallery.delete('hover-info')
        else:self.pixel_hover=None;self.canvas.delete('hover-info')

    def hover_gallery(self,event):
        self.gallery_hover=(event.x,event.y);self.show_gallery_hover()

    def show_gallery_hover(self):
        self.gallery.delete('hover-info')
        if self.gallery_hover is None:return
        x,y=self.gallery_hover
        for bounds,index in self.gallery_regions:
            x0,y0,x1,y1=bounds
            if x0<=x<x1 and y0<=y<y1:
                self.hover_label(self.gallery,x,y,f"{index}: {self.project.frames[index].name}");return

    def hover_pixel(self,event):
        self.pixel_hover=(event.x,event.y);self.show_pixel_hover()

    def show_pixel_hover(self):
        self.canvas.delete('hover-info')
        if self.pixel_hover is None or not self.project.frames or not hasattr(self,'hover_image'):return
        px,py=self.pixel_hover;ox,oy=self.offset;dw,dh=self.display_size
        if not (ox<=px<ox+dw and oy<=py<oy+dh):return
        f=self.project.frames[self.index]
        x=min(f.image.width-1,int((px-ox)*f.image.width/dw));y=min(f.image.height-1,int((py-oy)*f.image.height/dh))
        mode=self.view.get();alpha=f.image.getpixel((x,y))[3]
        if mode in ('metallic_smoothness','thickness','height','mask'):
            value=f.mask.getpixel((x,y)) if mode=='mask' else self.hover_image.getpixel((x,y))[0]
            name={'metallic_smoothness':'M&S','thickness':'Thickness','height':'Height','mask':'Mask'}[mode]
            text=f'{name}: {value/255:.3f} ({value}/255)  A:{alpha}'
        else:
            rgba=self.hover_image.getpixel((x,y));text='RGBA: '+', '.join(map(str,rgba))
        self.hover_label(self.canvas,px,py,f'({x}, {y})  '+text)

    def turn(self,delta):
        if self.guard():self.page=max(0,min((len(self.project.frames)-1)//48,self.page+delta));self.refresh()
    def select(self,e):
        if not self.guard():return
        col=e.x//110;row=e.y//83;i=self.page*48+row*6+col
        if col<6 and row<8 and i<len(self.project.frames):self.index=i;self.refresh()
    def paint(self,e):
        self.material_paint.cursor=(e.x,e.y)
        self.material_paint.shift_down=bool(e.state & 1)
        straight=bool(e.state & 1) and e.type==tk.EventType.ButtonPress
        if self.busy or not self.project.frames:return
        f=self.project.frames[self.index];x=int((e.x-self.offset[0])//self.zoom);y=int((e.y-self.offset[1])//self.zoom)
        if not (0<=x<f.image.width and 0<=y<f.image.height):return
        if self.material_ui.sample(f.image.getpixel((x,y))):return
        if self.view.get() in ('metallic_smoothness','thickness'):
            self.material_paint.paint(x,y,straight=straight,pressure=self.pen_input.sample(e));return
        if self.mode.get()=='pick':
            c=f.image.getpixel((x,y))
            if c[3] and list(c[:3]) not in self.project.colors:self.project.colors.append(list(c[:3]));self.dirty=True;self.refresh()
            return
        self.view.set('mask');draw=ImageDraw.Draw(f.mask);radius=int(self.params['brush'].get());value=255 if self.mode.get()=='paint' else 0
        if straight and self.material_paint.anchor() is not None:self.last_point=self.material_paint.anchor()
        if self.last_point:draw.line([self.last_point,(x,y)],fill=value,width=radius*2)
        draw.ellipse((x-radius,y-radius,x+radius,y+radius),fill=value);self.last_point=(x,y);self.material_paint.set_anchor(x,y);self.dirty=True;self.preview()
    def toggle_bridge(self):
        if not self.guard():return
        if self.bridge:
            self.bridge.close();self.bridge=None
            self.status.set(self.tr('直接連携を停止しました','Direct connection stopped'));return
        path=self.dialog('export_images','asksaveasfilename',title=self.tr('接続情報の保存先','Save connection details'),initialfile='sprite-editor-connection.json',defaultextension='.json')
        if not path:return
        from sprite_editor_bridge import EditorBridge
        try:
            self.bridge=EditorBridge(self)
            Path(path).write_text(json.dumps(self.bridge.connection(),indent=2),encoding='utf-8')
            self.status.set(self.tr('直接連携中：','Direct connection active: ')+path)
        except Exception as exc:
            if self.bridge:self.bridge.close();self.bridge=None
            messagebox.showerror('Connection',str(exc))

    def restore_session(self):
        path=self.paths.settings.parent/'sprite-editor-last-session.smproj'
        if not path.is_file():return
        def read(cb):
            project=Project.load(path,cb)
            with zipfile.ZipFile(path) as z:state=json.loads(z.read('editor-session.json'))
            return project,state
        def done(result):
            project,state=result;self.accept(project)
            self.index=max(0,min(int(state.get('index',0)),len(project.frames)-1))
            self.page=self.index//48
            view=state.get('view','source')
            self.view.set(view if view in ('source','mask','height','normal','metallic_smoothness','thickness') else 'source')
            self.dirty=bool(state.get('dirty',True))
            self.project_path=state.get('project_path')
        self.task(self.tr('前回の作業を復元','Restoring previous session'),read,done)

    def close(self):
        if not self.guard():return
        # Recovery is separate from the user's explicit project save.
        try:
            path=self.paths.settings.parent/'sprite-editor-last-session.smproj'
            path.parent.mkdir(parents=True,exist_ok=True)
            self.project.save(path,recovery=dict(index=self.index,view=self.view.get(),dirty=self.dirty,project_path=getattr(self,'project_path',None)))
        except Exception as exc:
            messagebox.showerror('Session recovery',str(exc),parent=self.root)
            return
        self.material_ui.close()
        if self.bridge:self.bridge.close()
        for browser in self.collection_browsers.values():browser.destroy()
        self.root.after_cancel(self.poll_job)
        if self.refresh_job:self.root.after_cancel(self.refresh_job)
        self.root.destroy()



if __name__=='__main__':
    import argparse
    args=argparse.ArgumentParser();args.add_argument('--collection');args.add_argument('--tag');args.add_argument('--project');options=args.parse_args()
    root=tk.Tk();editor=Editor(root)
    if options.project:root.after(100,lambda:editor.task(editor.tr('プロジェクト読込','Loading project'),lambda cb:Project.load(options.project,cb),lambda p:editor.loaded_project(p,options.project)))
    elif options.collection:root.after(100,lambda:editor.collection(options.collection,options.tag))
    else:root.after(100,editor.restore_session)
    root.mainloop()
