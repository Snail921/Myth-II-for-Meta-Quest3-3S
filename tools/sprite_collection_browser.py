"""Paged collection picker; decode off the Tk thread, one collection at a time."""
import queue
import threading
import tkinter as tk
from tkinter import ttk
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
        bar = ttk.Frame(self.window); bar.pack(fill='x')
        self.previous = ttk.Button(bar, text='◀', command=lambda: self.show(self.page-1)); self.previous.pack(side='left')
        self.status = ttk.Label(bar); self.status.pack(side='left', padx=12)
        self.next = ttk.Button(bar, text='▶', command=lambda: self.show(self.page+1)); self.next.pack(side='left')
        self.grid = ttk.Frame(self.window); self.grid.pack(fill='both', expand=True, padx=8, pady=8)
        self.window.protocol('WM_DELETE_WINDOW', self.close)
        self.show(0)
        self.job = self.window.after(60, self.poll)

    def show(self, page):
        self.page = page
        self.photos = []
        for child in self.grid.winfo_children(): child.destroy()
        self.previous.state(['disabled']); self.next.state(['disabled'])
        self.buttons = []
        batch = self.files[page*24:page*24+24]
        self.status.configure(text=f'{page+1}/{max(1,(len(self.files)+23)//24)} · 0/{len(batch)}')
        if not batch:
            self.status.configure(text=self.editor.tr('コレクションがありません', 'No collections found'))
        for i, path in enumerate(batch):
            cell = ttk.Frame(self.grid); cell.grid(row=i//6, column=i%6, padx=3, pady=3, sticky='nsew')
            button = ttk.Button(cell, text='…', state='disabled', command=lambda p=path: self.choose(p))
            button.pack(fill='both', expand=True)
            ttk.Label(cell, text=path.name, wraplength=140).pack()
            self.buttons.append(button)
        for i in range(6): self.grid.columnconfigure(i, weight=1, uniform='collection')
        for i in range(4): self.grid.rowconfigure(i, weight=1, uniform='collection')
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

    def choose(self, path):
        self.close()
        self.editor.collection(str(path))

    def reopen(self):
        self.window.title(self.editor.tr('コレクション一覧', 'Collection browser'))
        self.window.deiconify()
        self.window.lift()
    def close(self):
        # Keep the page, widgets and thumbnails alive for this editor session.
        self.window.withdraw()
    def destroy(self):
        self.stop.set()
        self.window.after_cancel(self.job)
        self.window.destroy()
