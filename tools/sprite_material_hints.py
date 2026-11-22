"""Named representative colors: authoring metadata, never runtime normals."""
from tkinter import ttk, colorchooser
import tkinter as tk


def validate_hints(value):
    if not isinstance(value, list) or len(value) != 5:
        raise ValueError('Expected five material hints')
    result = []
    for row in value:
        if not isinstance(row, dict) or not isinstance(row.get('name'), str) or len(row['name']) > 120:
            raise ValueError('Invalid material name')
        colors = row.get('colors')
        if not isinstance(colors, list) or len(colors) != 3:
            raise ValueError('Expected three color slots')
        for c in colors:
            if c is not None and (not isinstance(c, list) or len(c) != 3 or any(type(v) is not int or not 0 <= v <= 255 for v in c)):
                raise ValueError('Invalid RGB sample')
        result.append(dict(name=row['name'], colors=[None if c is None else list(c) for c in colors]))
    return result


class MaterialHints:
    def __init__(self, editor):
        self.editor = editor
        self.window = None
        self.pick_target = None

    def open(self):
        e = self.editor
        if not e.guard(): return
        if self.window is not None and self.window.winfo_exists():
            self.window.lift(); return
        w = self.window = tk.Toplevel(e.root)
        w.title(e.tr('材質の代表色', 'Material color references'))
        w.geometry('780x350')
        w.attributes('-topmost', True)
        self.names = []; self.buttons = []
        ttk.Label(w, text=e.tr('材質名と代表色を登録。各色の「採取」を押して元画像をクリックします。',
                              'Name materials and register colors. Use Pick, then click the source image.')).pack(pady=8)
        grid = ttk.Frame(w, padding=8); grid.pack(fill='x')
        grid.columnconfigure(0, weight=1)
        for i in range(5):
            var = tk.StringVar(value=e.project.material_hints[i]['name']); self.names.append(var)
            ttk.Entry(grid, textvariable=var, width=24).grid(row=i, column=0, sticky='ew', pady=5)
            var.trace_add('write', lambda *_, row=i: self.rename(row))
            buttons = []
            for j in range(3):
                b = tk.Button(grid, width=9, command=lambda row=i, slot=j: self.choose(row, slot))
                b.grid(row=i, column=1+j*3, padx=3); buttons.append(b)
                ttk.Button(grid, text=e.tr('採取','Pick'), width=5,
                           command=lambda row=i, slot=j: self.arm(row, slot)).grid(row=i, column=2+j*3)
                ttk.Button(grid, text='×', width=2,
                           command=lambda row=i, slot=j: self.set_color(row, slot, None)).grid(row=i, column=3+j*3)
            self.buttons.append(buttons)
        ttk.Label(w, text=e.tr('プロジェクト保存に含まれます。既存のハイト用マスクやQuest用ノーマルは変更しません。',
                              'Saved with the project. Existing height masks and Quest normals are unchanged.'), wraplength=740).pack(pady=8)
        w.protocol('WM_DELETE_WINDOW', self.close)
        self.update()

    def close(self):
        self.pick_target = None
        if self.window is not None: self.window.destroy()
        self.window = None

    def rename(self, row):
        e = self.editor
        if e.busy: return
        value = self.names[row].get()[:120]
        if e.project.material_hints[row]['name'] != value:
            e.project.material_hints[row]['name'] = value; e.dirty = True

    def set_color(self, row, slot, value):
        e = self.editor
        if not e.guard(): return
        e.project.material_hints[row]['colors'][slot] = value
        e.dirty = True; self.update()

    def choose(self, row, slot):
        if not self.editor.guard(): return
        c = self.editor.project.material_hints[row]['colors'][slot]
        value = colorchooser.askcolor(color='#%02x%02x%02x' % tuple(c or [128]*3), parent=self.window)[0]
        if value is not None: self.set_color(row, slot, [int(round(v)) for v in value])

    def arm(self, row, slot):
        if not self.editor.guard(): return
        self.pick_target = (row, slot)
        self.editor.view.set('source'); self.editor.refresh()
        self.editor.status.set(self.editor.tr('元画像をクリックして代表色を採取（Escで中止）', 'Click the source image to sample a color (Esc cancels)'))

    def sample(self, rgba):
        if self.pick_target is None: return False
        if rgba[3]:
            row, slot = self.pick_target; self.pick_target = None
            self.set_color(row, slot, list(rgba[:3]))
        return True

    def update(self):
        if self.window is None: return
        for i, row in enumerate(self.editor.project.material_hints):
            for j, c in enumerate(row['colors']):
                color = '#%02x%02x%02x' % tuple(c) if c is not None else '#dddddd'
                self.buttons[i][j].configure(text=color if c is not None else '—', bg=color,
                    fg='white' if c is not None and sum(c)<360 else 'black')
