"""Paged collection picker; decode off the Tk thread, one collection at a time."""
import queue
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
from PIL import ImageTk
from sprite_editor_files import collections, thumbnail


class CollectionBrowser:
    def __init__(self, editor, folder):
        self.editor = editor
        self.files = collections(folder)
        self.cache = {}
        self.page = 0
        self.closed = False
        self.stop = threading.Event()
        self.events = queue.Queue()
        self.photos = []
        self.window = tk.Toplevel(editor.root)
        self.window.title(editor.tr('コレクション一覧', 'Collection browser'))
        self.window.geometry('940x770')
        ttk.Label(self.window, text=str(folder), wraplength=900).pack(fill='x', padx=8, pady=6)
        self.export_paths = {}
        for key, ja, en in [('normal_export_folder', '通常ノーマル出力フォルダ', 'Normal export folder'), ('pbr_export_folder', 'PBR出力フォルダ', 'PBR export folder')]:
            row = ttk.Frame(self.window); row.pack(fill='x', padx=8, pady=2)
            ttk.Label(row, text=editor.tr(ja, en), width=23).pack(side='left')
            value = tk.StringVar(value=editor.paths.paths.get(key, ''))
            self.export_paths[key] = value
            ttk.Entry(row, textvariable=value, state='readonly').pack(side='left', fill='x', expand=True)
            ttk.Button(row, text=editor.tr('参照', 'Browse'), command=lambda k=key: self.select_export_folder(k)).pack(side='left')
        help_row = ttk.Frame(self.window); help_row.pack(fill='x', padx=8)
        ttk.Label(help_row, text=editor.tr('✓ 同名.mvrnmあり / — なし / ? 未設定・参照不可（フォルダ直下を確認）', '✓ Matching .mvrnm exists / — Missing / ? Unset or unavailable (direct folder only)')).pack(side='left')
        ttk.Button(help_row, text=editor.tr('更新', 'Refresh'), command=self.refresh_exports).pack(side='right')
        self.export_labels = []
        bar = ttk.Frame(self.window); bar.pack(fill='x')
        self.previous = ttk.Button(bar, text='◀', command=lambda: self.show(self.page-1)); self.previous.pack(side='left')
        self.status = ttk.Label(bar); self.status.pack(side='left', padx=12)
        self.next = ttk.Button(bar, text='▶', command=lambda: self.show(self.page+1)); self.next.pack(side='left')
        viewport = ttk.Frame(self.window); viewport.pack(fill='both', expand=True, padx=8, pady=8)
        self.scroll = tk.Canvas(viewport, highlightthickness=0)
        scrollbar = ttk.Scrollbar(viewport, orient='vertical', command=self.scroll.yview)
        scrollbar.pack(side='right', fill='y'); self.scroll.pack(side='left', fill='both', expand=True)
        self.scroll.configure(yscrollcommand=scrollbar.set)
        self.grid = ttk.Frame(self.scroll)
        self.grid_window = self.scroll.create_window(0, 0, window=self.grid, anchor='nw')
        self.grid.bind('<Configure>', lambda event:self.scroll.configure(scrollregion=self.scroll.bbox('all')))
        self.scroll.bind('<Configure>', lambda event:self.scroll.itemconfigure(self.grid_window, width=event.width))
        self.window.bind('<MouseWheel>', self.scroll_wheel)

        self.window.protocol('WM_DELETE_WINDOW', self.close)
        self.show(0)
        self.job = self.window.after(60, self.poll)

    def show(self, page):
        self.page = page
        self.photos = []
        for child in self.grid.winfo_children(): child.destroy()
        self.previous.state(['disabled']); self.next.state(['disabled'])
        self.buttons = []
        self.export_labels = []
        batch = self.files[page*24:page*24+24]
        self.status.configure(text=f'{page+1}/{max(1,(len(self.files)+23)//24)} · 0/{len(batch)}')
        if not batch:
            self.status.configure(text=self.editor.tr('コレクションがありません', 'No collections found'))
        for i, path in enumerate(batch):
            cell = ttk.Frame(self.grid); cell.grid(row=i//6, column=i%6, padx=3, pady=3, sticky='nsew')
            badges = ttk.Frame(cell); badges.pack(fill='x', pady=(0, 4))
            labels = []
            for title in ('Normal', 'PBR'):
                label = tk.Label(badges, text=title, anchor='center', padx=3, pady=2,
                                 font=('TkDefaultFont', 9, 'bold'))
                label.pack(side='left', fill='x', expand=True, padx=1)
                labels.append(label)
            button = ttk.Button(cell, text='…', state='disabled', command=lambda p=path: self.choose(p))
            button.pack(fill='both', expand=True)
            ttk.Label(cell, text=path.name, wraplength=140).pack()
            self.export_labels.append((path, labels))
            self.buttons.append(button)
        for i in range(6): self.grid.columnconfigure(i, weight=1, uniform='collection')
        for i in range(4): self.grid.rowconfigure(i, weight=1, uniform='collection')
        self.scroll.yview_moveto(0)
        self.refresh_exports()
        def work():
            for i, path in enumerate(batch):
                if self.stop.is_set(): return
                if path in self.cache:result = self.cache[path]
                else:
                    try: result = thumbnail(path)
                    except Exception as exc: result = str(exc)
                    self.cache[path] = result
                self.events.put((i, result))
            self.events.put((None, None))
        threading.Thread(target=work, daemon=True).start()

    def poll(self):
        try:
            while True:
                i, result = self.events.get_nowait()
                if i is None:
                    if self.page > 0: self.previous.state(['!disabled'])
                    if (self.page+1)*24 < len(self.files): self.next.state(['!disabled'])
                    continue
                if isinstance(result, str):
                    self.buttons[i].configure(text=self.editor.tr('読込不可', 'Cannot preview'))
                else:
                    photo = ImageTk.PhotoImage(result, master=self.window); self.photos.append(photo)
                    self.buttons[i].configure(image=photo, text=''); self.buttons[i].state(['!disabled'])
                self.status.configure(text=f'{self.page+1}/{max(1,(len(self.files)+23)//24)} · {i+1}/{len(self.buttons)}')
        except queue.Empty: pass
        self.job = self.window.after(60, self.poll)

    def scroll_wheel(self, event):
        if self.grid.winfo_height() > self.scroll.winfo_height():
            self.scroll.yview_scroll(-int(event.delta / 120), 'units')
            return 'break'

    def select_export_folder(self, key):
        selected = filedialog.askdirectory(parent=self.window, initialdir=self.editor.paths.initial(key))
        if not selected: return
        try:
            self.editor.paths.remember(key, selected, directory=True)
        except OSError as exc:
            messagebox.showerror('Export folders', str(exc), parent=self.window)
            return
        self.export_paths[key].set(selected)
        self.refresh_exports()

    def refresh_exports(self):
        from sprite_editor_files import exported_collection_names
        indices = [exported_collection_names(self.export_paths[key].get())
                   for key in ('normal_export_folder', 'pbr_export_folder')]
        for path, labels in self.export_labels:
            marks = ['?' if names is None else ('✓' if path.stem.casefold() in names else '—') for names in indices]
            for label, title, mark, color in zip(labels, ('Normal','PBR'), marks, ('#1565c0','#7b1fa2')):
                background = color if mark == '✓' else ('#b45309' if mark == '?' else '#e0e0e0')
                label.configure(text=f'{title} {mark}', background=background,
                                foreground='#333333' if mark == '—' else 'white')

    def choose(self, path):
        self.close()
        self.editor.collection(str(path))

    def reopen(self):
        self.window.title(self.editor.tr('コレクション一覧', 'Collection browser'))
        self.refresh_exports()
        self.window.deiconify()
        self.window.lift()
    def close(self):
        # Keep the page, widgets and thumbnails alive for this editor session.
        self.window.withdraw()
    def destroy(self):
        self.stop.set()
        self.window.after_cancel(self.job)
        self.window.destroy()
