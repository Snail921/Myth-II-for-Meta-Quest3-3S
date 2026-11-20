"""Local Windows GUI for the supported single-plugin workflow."""
import json,os,queue,threading,uuid,time
from build_progress import PREFIX
import manager_i18n as i18n
from level_modes import mode_label
from pathlib import Path
# Local Tcl scripts also work when the host Python installation is inaccessible
# to the embedded Tcl filesystem. DLLs remain supplied by Python 3.11.
_local_tcl=Path(__file__).resolve().parent/'runtime/plugin-manager-tcl'
if (_local_tcl/'tcl8.6/init.tcl').is_file():
 os.environ['TCL_LIBRARY']=str(_local_tcl/'tcl8.6');os.environ['TK_LIBRARY']=str(_local_tcl/'tk8.6')
import tkinter as tk
from tkinter import ttk,filedialog,messagebox
from plugin_manager_backend import ROOT,ADB_DEFAULT,Device,run_conversion,bundle_inputs
class App:
 def __init__(self,root):
  self.language=i18n.load();self.localized=[];self.hd=[]
  self.root=root;root.title('Myth II VR — プラグイン管理');root.geometry('1100x760');root.minsize(850,650);self.queue=queue.Queue();self.busy=False;self.buttons=[];self.last_progress=0;self.stage_text='';self.last_log_time=0
  self.game=tk.StringVar(value=str(ROOT/'UserData/Game'));self.plugin=tk.StringVar();self.output=tk.StringVar(value=str(ROOT/'build/plugin-manager'));self.bundle=tk.StringVar();self.adb=tk.StringVar(value=str(ADB_DEFAULT));self.serial=tk.StringVar();self.status=tk.StringVar(value='原作フォルダとプラグインを選び、検査してください。')
  common=ttk.Frame(root,padding=(12,8));common.pack(fill='x');common.columnconfigure(1,weight=1)
  for row,(title,var,folder) in enumerate([('作業・保存先',self.output,True),('ADB',self.adb,False)]):
   ttk.Label(common,text=title).grid(row=row,column=0,sticky='w');ttk.Entry(common,textvariable=var).grid(row=row,column=1,sticky='ew',padx=6,pady=3)
   self.button(common,'参照',lambda v=var,d=folder:self.browse(v,d)).grid(row=row,column=2)
  self.common=common
  self.panes=ttk.Panedwindow(root,orient='vertical');self.panes.pack(fill='both',expand=True,padx=12,pady=4)
  footer=ttk.Frame(self.panes,padding=(0,4));footer.columnconfigure(0,weight=1);footer.rowconfigure(0,weight=1)
  self.notebook=ttk.Notebook(self.panes)
  self.panes.add(self.notebook,weight=0);self.panes.add(footer,weight=1)
  self.setup_tab=ttk.Frame(self.notebook,padding=8);self.plugin_tab=ttk.Frame(self.notebook,padding=8)
  self.notebook.add(self.setup_tab,text='セットアップ');self.notebook.add(self.plugin_tab,text='プラグイン・素材')
  frame=self.plugin_tab;frame.columnconfigure(1,weight=1)
  ttk.Label(frame,text='1.8.5 / プラグイン欄が空なら原作マップ。シングル・対戦を変換します。').grid(row=0,column=0,columnspan=3,sticky='w',pady=(0,10))
  for row,(title,var,folder) in enumerate([('原作フォルダ',self.game,True),('プラグインファイル',self.plugin,False),('変換済み bundle',self.bundle,True)],1):
   ttk.Label(frame,text=title).grid(row=row,column=0,sticky='w');ttk.Entry(frame,textvariable=var).grid(row=row,column=1,sticky='ew',padx=6,pady=3)
   self.button(frame,'参照',lambda v=var,d=folder:self.browse(v,d)).grid(row=row,column=2)
  ttk.Label(common,text='Quest シリアル').grid(row=2,column=0,sticky='w');self.devices=ttk.Combobox(common,textvariable=self.serial);self.devices.grid(row=2,column=1,sticky='ew',padx=6);self.button(common,'接続確認',self.refresh).grid(row=2,column=2)
  bar=ttk.Frame(frame);bar.grid(row=7,column=0,columnspan=3,sticky='w',pady=10)
  for title,command in [('① レベルを検査',self.inspect),('② 全レベルを変換',self.convert),('③ Questへ配置',self.deploy),('保存先を開く',self.open_output)]:self.button(bar,title,command).pack(side='left',padx=(0,8))
  self.tree=ttk.Treeview(frame,columns=('id','name','mode','status'),show='headings',height=9)
  for col,title,width in [('id','ID',95),('name','レベル',260),('mode','種類',110),('status','検査結果',430)]:self.tree.heading(col,text=title);self.tree.column(col,width=width)
  self.tree.grid(row=8,column=0,columnspan=3,sticky='nsew');frame.rowconfigure(8,weight=1)
  scroll=ttk.Scrollbar(frame,orient='vertical',command=self.tree.yview);scroll.grid(row=8,column=3,sticky='ns');self.tree.configure(yscrollcommand=scroll.set)
  self.text=tk.Text(footer,height=3,wrap='word');self.text.grid(row=0,column=0,sticky='nsew',pady=4)
  scroll=ttk.Scrollbar(footer,orient='vertical',command=self.text.yview);scroll.grid(row=0,column=1,sticky='ns');self.text.configure(yscrollcommand=scroll.set)
  style=ttk.Style(root);style.theme_use('clam');style.configure('Blue.Horizontal.TProgressbar',background='#2687e8',troughcolor='#e0e7ef')
  self.progress=ttk.Progressbar(footer,style='Blue.Horizontal.TProgressbar',mode='indeterminate');self.progress.grid(row=1,column=0,columnspan=2,sticky='ew',pady=3)
  ttk.Label(footer,textvariable=self.status,wraplength=920).grid(row=2,column=0,columnspan=2,sticky='w')
  ttk.Label(frame,text='配置は原作ゲームを終了してから。旧パックは退避され、APKの更新は不要です。').grid(row=12,column=0,columnspan=3,sticky='w',pady=5)
  self.button(bar,'スプライト材質エディター',self.material_editor).pack(side='left',padx=(0,8))
  self.material_folder=tk.StringVar(value=str(ROOT/'UserData/Materials'))
  materials=ttk.Frame(frame);materials.grid(row=13,column=0,columnspan=3,sticky='ew',pady=5);materials.columnconfigure(1,weight=1)
  ttk.Label(materials,text='ノーマルマップフォルダ').grid(row=0,column=0,sticky='w')
  ttk.Entry(materials,textvariable=self.material_folder).grid(row=0,column=1,sticky='ew',padx=6)
  self.button(materials,'参照',lambda:self.browse(self.material_folder,True)).grid(row=0,column=2,padx=4)
  self.button(materials,'ノーマルマップをQuestへ転送',self.deploy_materials).grid(row=0,column=3)
  ttk.Label(frame,text='フォルダ直下の.mvrnmを一括転送。同名ファイルは更新します。').grid(row=14,column=0,columnspan=3,sticky='w')
  self.add_setup()
  language=ttk.Combobox(common,values=['日本語','English'],state='readonly',width=12);language.set('English' if self.language=='en' else '日本語');language.grid(row=0,column=3,sticky='e',padx=8);language.bind('<<ComboboxSelected>>',lambda e:self.set_language('en' if language.get()=='English' else 'ja'))
  self.capture_labels(root);self.set_language(self.language,False)
  root.after(100,self.initial_split)
  root.after(100,self.poll);root.protocol('WM_DELETE_WINDOW',self.close)
 def initial_split(self):
  self.root.update_idletasks()
  self.panes.sashpos(0,min(self.setup_tab.winfo_reqheight()+32,max(0,self.panes.winfo_height()-85)))
 def deploy_materials(self):
  from sprite_material_deploy import deploy
  folder,adb,serial=self.material_folder.get().strip(),self.adb.get(),self.serial.get()
  if not folder:self.error('入力','ノーマルマップのフォルダを指定してください。');return
  self.task(lambda:deploy(folder,Device(adb,serial,self.log)))
 def material_editor(self):
  from sprite_material_editor import Editor
  window=tk.Toplevel(self.root);editor=Editor(window);editor.lang.set("English" if self.language=="en" else "日本語");editor.translate()
 def error(self,title,message):messagebox.showerror(self.tr(title),self.tr(message))
 def info(self,title,message):messagebox.showinfo(self.tr(title),self.tr(message))
 def tr(self,text):return i18n.translate(text,self.language)
 def capture_labels(self,widget):
  if isinstance(widget,(ttk.Label,ttk.Button,ttk.LabelFrame)) and widget.cget('text'):self.localized.append((widget,widget.cget('text')))
  for child in widget.winfo_children():self.capture_labels(child)
 def set_language(self,language,persist=True):
  self.language=language
  self.notebook.tab(self.setup_tab,text="Setup" if language=="en" else "セットアップ")
  self.notebook.tab(self.plugin_tab,text="Plugins & materials" if language=="en" else "プラグイン・素材")
  if persist:i18n.save(language)
  self.root.title('Myth II VR — '+self.tr('プラグイン管理'))
  for widget,text in self.localized:widget.configure(text=self.tr(text))
  for col,title in [('id','ID'),('name','レベル'),('mode','種類'),('status','検査結果')]:self.tree.heading(col,text=self.tr(title))
  for item in self.tree.get_children():
   values=list(self.tree.item(item,'values'))
   for mode in ('single','multiplayer','both','unknown'):
    label=mode_label(mode)
    if len(values)>2 and values[2] in (label,i18n.translate(label,'en')):values[2]=self.tr(label);break
   self.tree.item(item,values=values)
  if not self.busy:self.status.set(self.tr('原作フォルダとプラグインを選び、検査してください。'))
 def add_setup(self):
  self.skyboxes=tk.StringVar();self.logo=tk.StringVar();self.cd_face=tk.StringVar();self.iso=tk.StringVar();self.updater=tk.StringVar();self.seven=tk.StringVar(value='C:/Program Files/7-Zip/7z.exe');self.setup_result=tk.StringVar();self.seed=tk.StringVar()
  frame=ttk.LabelFrame(self.setup_tab,text='原作セットアップ（シングル・マルチ）',padding=8);frame.pack(fill='x');frame.columnconfigure(1,weight=1)
  for row,(title,var,folder) in enumerate([('ISOファイル',self.iso,False),('1.8.5アップデーター',self.updater,False),('7-Zip',self.seven,False),('ランタイムデータ',self.seed,False),('スカイボックス（任意）',self.skyboxes,True),('ロゴ画像（任意）',self.logo,False),('CD画像（任意）',self.cd_face,False)]):
   ttk.Label(frame,text=title).grid(row=row,column=0,sticky='w');ttk.Entry(frame,textvariable=var).grid(row=row,column=1,sticky='ew');self.button(frame,'参照',lambda v=var,d=folder:self.browse(v,d)).grid(row=row,column=2)
  self.button(frame,'任意のHDアドオンを追加',self.add_hd).grid(row=7,column=0,columnspan=3,sticky='w',pady=(6,2))
  self.button(frame,'原作を展開・全レベルを変換',self.setup_convert).grid(row=8,column=0,columnspan=3,sticky='w',pady=2)
  ttk.Label(frame,text='セットアップ結果').grid(row=9,column=0,sticky='w')
  ttk.Entry(frame,textvariable=self.setup_result).grid(row=9,column=1,sticky='ew',pady=2)
  self.button(frame,'参照',lambda:self.browse(self.setup_result,True)).grid(row=9,column=2)
  self.button(frame,'セットアップデータをQuestに転送',self.setup_deploy).grid(row=10,column=0,columnspan=3,sticky='w',pady=(2,0))

 def add_hd(self):
  self.hd=list(filedialog.askopenfilenames());self.log('HD addons: '+', '.join(self.hd))
 def setup_convert(self):
  from plugin_manager_backend import run_setup
  iso,updater,seven=self.iso.get(),self.updater.get(),self.seven.get();hd=list(self.hd)
  if not all((iso,updater,seven)):self.error('Input','Select ISO, updater and 7-Zip.');return
  skyboxes,logo,cd_face=self.skyboxes.get(),self.logo.get(),self.cd_face.get()
  dest=Path(self.output.get())/('setup-'+uuid.uuid4().hex[:12]);self.setup_result.set(str(dest))
  def work():
   result=run_setup(iso,updater,seven,hd,dest,self.log,skyboxes,logo,cd_face);self.queue.put(('game',str(dest/'Game')));return result
  self.task(work)
 def setup_deploy(self):
  from setup_deploy import deploy
  folder,seed,adb,serial=self.setup_result.get(),self.seed.get(),self.adb.get(),self.serial.get()
  self.task(lambda:deploy(folder,seed,Device(adb,serial,self.log)))
 def button(self,parent,text,command):
  b=ttk.Button(parent,text=text,command=command);self.buttons.append(b);return b
 def browse(self,var,directory):
  if var is self.logo or var is self.cd_face:
   path=filedialog.askopenfilename(filetypes=[('PNG','*.png')])
  else:path=filedialog.askdirectory() if directory else filedialog.askopenfilename()
  if path:var.set(path)
 def log(self,text):self.queue.put(('log',text))
 def task(self,fn):
  if self.busy:return
  self.busy=True;self.started=time.monotonic();self.last_progress=self.started;self.last_log_time=0;self.stage_text='処理を開始しています';self.status.set(self.tr(self.stage_text));self.progress.configure(mode='indeterminate',value=0);self.progress.start(15)
  for b in self.buttons:b.state(['disabled'])
  def worker():
   try:self.queue.put(('done',fn()))
   except Exception as e:self.queue.put(('error',str(e)))
  threading.Thread(target=worker,daemon=False).start()
 def inputs(self):
  game=self.game.get().strip();plugin=self.plugin.get().strip();out=self.output.get().strip()
  if not game or not out:raise ValueError('原作フォルダ・保存先を指定してください。')
  return game,plugin,Path(out).resolve()
 def inspect(self):self.start_convert(True)
 def convert(self):self.start_convert(False)
 def start_convert(self,inspect):
  try:game,plugin,out=self.inputs()
  except Exception as e:self.error('入力',str(e));return
  destination=out/(('inspect-' if inspect else 'convert-')+uuid.uuid4().hex[:12])
  def work():
   result=run_conversion(game,plugin,destination,inspect,self.log)
   if inspect:self.queue.put(('levels',destination))
   else:self.queue.put(('bundle',str(result)));self.queue.put(('levels',destination))
   return '検査が完了しました。結果を確認してください。' if inspect else '変換が完了しました。原作を終了して「Questへ配置」を押してください。'
  self.task(work)
 def deploy(self):
  bundle=self.bundle.get().strip();plugin=self.plugin.get().strip();out=Path(self.output.get()).resolve();adb=self.adb.get();serial=self.serial.get()
  if not bundle:self.error('入力','bundleフォルダを指定してください。');return
  self.task(lambda:'配置完了。記録: '+str(Device(adb,serial,self.log).deploy(bundle,plugin,out/'deployments')))
 def refresh(self):
  adb=self.adb.get()
  def work():
   ids=Device(adb).devices();self.queue.put(('devices',ids));return '接続済み: '+(', '.join(ids) or 'なし。USBデバッグ許可をご確認ください。')
  self.task(work)
 def open_output(self):
  try:p=Path(self.output.get()).resolve();p.mkdir(parents=True,exist_ok=True);os.startfile(p)
  except Exception as e:self.error('保存先',str(e))
 def poll(self):
  try:
   while True:
    kind,value=self.queue.get_nowait()
    if kind=='log':
     if value.startswith(PREFIX):
      try:data=json.loads(value[len(PREFIX):])
      except ValueError:data=None
      if data is not None:
       current,total=data.get('current'),data.get('total');stage=self.tr(data['stage']);item=data.get('item','')
       changed=not self.stage_text.startswith(stage);self.stage_text=stage
       self.progress.stop()
       if total is not None and total>0 and current is not None:
        self.progress.configure(mode='determinate',maximum=total,value=current);self.stage_text+=f' ({current:03d}/{total:03d})'
       else:self.progress.configure(mode='indeterminate',value=0);self.progress.start(15)
       if item:self.stage_text+=' — '+item
       self.last_progress=time.monotonic()
       if changed or self.last_progress-self.last_log_time>=1 or current==total:
        self.text.insert('end',self.stage_text+'\n');self.last_log_time=self.last_progress
      else:self.text.insert('end',self.tr(value)+'\n')
     else:self.text.insert('end',self.tr(value)+'\n')
     self.text.see('end')
    elif kind in ('done','error'):
     self.busy=False;self.progress.stop();self.progress.configure(mode='determinate',maximum=1,value=1 if kind=='done' else 0)
     for b in self.buttons:b.state(['!disabled'])
     self.status.set(self.tr(value))
     if kind=='error':self.text.insert('end','エラー: '+value+'\n');self.error('処理を完了できませんでした',value)
    elif kind=='bundle':self.bundle.set(value)
    elif kind=='game':self.game.set(value)
    elif kind=='devices':self.devices['values']=value;self.serial.set(value[0] if len(value)==1 else '')
    elif kind=='levels':
     self.tree.delete(*self.tree.get_children());rows=json.loads((value/'preflight.json').read_text(encoding='utf8'))
     for key,row in rows.items():self.tree.insert('','end',values=(key,row['name'],self.tr(mode_label(row.get('mode','unknown'))),self.tr('対応可能' if row['supported'] else ('不足素材: '+str(row['missing_dependencies']) if row['missing_dependencies'] else '素材数が上限223を超えています'))))
     for row in json.loads((value/'excluded-entrypoints.json').read_text(encoding='utf8')):self.tree.insert('','end',values=(row['key'],row['name'],self.tr(mode_label(row.get('mode','unknown'))),self.tr('除外（対応モードなし）' if row.get('mode')=='unknown' else '除外（未完成のエントリー）')))
  except queue.Empty:pass
  if self.busy:
   elapsed=int(time.monotonic()-self.started);quiet=int(time.monotonic()-self.last_progress)
   self.status.set(self.stage_text+f' | {self.tr("経過")} {elapsed//60:02d}:{elapsed%60:02d}'+(f' | {self.tr("進捗通知待ち")} {quiet}s' if quiet>=5 else ''))
  self.root.after(100,self.poll)
 def close(self):
  if self.busy:self.info('処理中','処理が終わるまでお待ちください。変換ログは保存先にも記録されています。')
  else:self.root.destroy()
if __name__=='__main__':
 root=tk.Tk();App(root);root.mainloop()
