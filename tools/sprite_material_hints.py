"""Named representative colors used by the editor and the local material API."""
from tkinter import ttk, colorchooser
import tkinter as tk


HINT_COUNT = 10
SLOT_COUNT = 10


def empty_hint():
    return dict(name='', colors=[None]*SLOT_COUNT, samples=[[] for _ in range(SLOT_COUNT)])


def average(colors):
    if not colors:
        return None
    return [round(sum(channel)/len(colors)) for channel in zip(*colors)]


def valid_rgb(value):
    return isinstance(value, list) and len(value) == 3 and all(type(v) is int and 0 <= v <= 255 for v in value)


def validate_hints(value):
    if not isinstance(value, list) or len(value) not in (5, HINT_COUNT):
        raise ValueError('Invalid material hint count')
    result = []
    for row in value:
        if not isinstance(row, dict) or not isinstance(row.get('name'), str) or len(row['name']) > 120:
            raise ValueError('Invalid material name')
        colors = row.get('colors')
        if not isinstance(colors, list) or len(colors) not in (3, SLOT_COUNT):
            raise ValueError('Invalid color slot count')
        if any(c is not None and not valid_rgb(c) for c in colors):
            raise ValueError('Invalid representative RGB')
        samples = row.get('samples', [[] if c is None else [c] for c in colors])
        if not isinstance(samples, list) or len(samples) != len(colors):
            raise ValueError('Invalid sample slots')
        hint = empty_hint()
        hint['name'] = row['name']
        for j, group in enumerate(samples):
            if not isinstance(group, list) or not all(valid_rgb(c) for c in group):
                raise ValueError('Invalid RGB samples')
            group = [list(c) for c in dict.fromkeys(map(tuple, group))]
            hint['samples'][j] = group
            hint['colors'][j] = average(group)
        result.append(hint)
    result.extend(empty_hint() for _ in range(HINT_COUNT-len(result)))
    return result


class MaterialHints:
    def __init__(self, editor):
        self.editor = editor
        self.window = None
        self.cancel()

    def open(self):
        e = self.editor
        if not e.guard(): return
        if self.window is not None and self.window.winfo_exists():
            self.window.lift(); return
        w = self.window = tk.Toplevel(e.root)
        w.title(e.tr('材質の代表色', 'Material color references'))
        w.attributes('-topmost', True)
        self.names = []; self.buttons = []
        ttk.Label(w, text=e.tr('色枠を選び、右の元画像をドラッグして登録。表示は平均色と色数。右クリックで色指定・消去。',
                              'Select a slot and drag on the source preview. Each slot shows the average and distinct color count. Right-click to choose or clear.')).pack(pady=6)
        grid = ttk.Frame(w, padding=6); grid.pack(fill='both',expand=True)
        self.menu = tk.Menu(w, tearoff=False)
        for i, hint in enumerate(e.project.material_hints):
            var = tk.StringVar(value=hint['name']); self.names.append(var)
            ttk.Entry(grid, textvariable=var, width=19).grid(row=i, column=0, sticky='ew', pady=2)
            var.trace_add('write', lambda *_, row=i: self.rename(row))
            buttons = []
            for j in range(SLOT_COUNT):
                b = tk.Button(grid, width=8, height=2, command=lambda row=i, slot=j: self.arm(row, slot))
                b.grid(row=i, column=j+1, padx=1, pady=1)
                b.bind('<Button-3>', lambda ev,row=i,slot=j:self.slot_menu(ev,row,slot))
                buttons.append(b)
            self.buttons.append(buttons)
        w.bind('<Escape>', lambda _: self.cancel())
        w.protocol('WM_DELETE_WINDOW', self.close)
        self.update()

    def cancel(self):
        self.pick_target = None
        self.last = None
        self.pending = {}

    def close(self):
        self.cancel()
        if self.window is not None: self.window.destroy()
        self.window = None

    def rename(self, row):
        e = self.editor
        if e.busy: return
        value = self.names[row].get()[:120]
        if e.project.material_hints[row]['name'] != value:
            e.project.material_hints[row]['name'] = value; e.dirty = True

    def store(self, row, slot, samples):
        hint = self.editor.project.material_hints[row]
        if samples != hint['samples'][slot]:
            hint['samples'][slot] = samples
            hint['colors'][slot] = average(samples)
            self.editor.dirty = True
        self.update_slot(row, slot)

    def slot_menu(self,event,row,slot):
        if not self.editor.guard(): return
        self.cancel()
        self.menu.delete(0,'end')
        self.menu.add_command(label=self.editor.tr('色を指定','Choose color'),command=lambda:self.choose(row,slot))
        self.menu.add_command(label=self.editor.tr('消去','Clear'),command=lambda:self.store(row,slot,[]))
        try:
            self.menu.tk_popup(event.x_root,event.y_root)
        finally:
            self.menu.grab_release()

    def choose(self, row, slot):
        c = self.editor.project.material_hints[row]['colors'][slot]
        value = colorchooser.askcolor(color='#%02x%02x%02x' % tuple(c or [128]*3), parent=self.window)[0]
        if value is not None: self.store(row, slot, [[round(v) for v in value]])

    def arm(self, row, slot):
        if not self.editor.guard(): return
        self.cancel()
        self.pick_target = (row, slot)
        self.editor.view.set('source'); self.editor.refresh()
        self.editor.status.set(self.editor.tr('右の元画像をドラッグして採取（Escで中止）', 'Drag on the source preview to sample colors (Esc cancels)'))

    def sample(self, image, x, y):
        if self.pick_target is None: return False
        if not (0 <= x < image.width and 0 <= y < image.height):
            self.last = None
            return True
        u,v = self.last or (x,y)
        steps = max(1,abs(x-u),abs(y-v))
        for i in range(steps+1):
            pixel = image.getpixel((round(u+(x-u)*i/steps),round(v+(y-v)*i/steps)))
            if pixel[3]: self.pending[pixel[:3]] = None
        self.last = (x,y)
        return True

    def finish(self):
        if self.pick_target is None: return False
        if self.pending:
            self.store(*self.pick_target,[list(c) for c in self.pending])
        self.cancel()
        return True

    def update_slot(self,i,j):
        if self.window is None: return
        hint = self.editor.project.material_hints[i]
        c = hint['colors'][j]
        count = len(hint['samples'][j])
        color = '#%02x%02x%02x' % tuple(c) if c is not None else '#dddddd'
        label = color+'\n'+self.editor.tr(f'{count}色',f'{count} colors') if count else '—'
        self.buttons[i][j].configure(text=label, bg=color, fg='white' if c is not None and sum(c)<360 else 'black')

    def update(self):
        for i,buttons in enumerate(self.buttons):
            for j in range(len(buttons)): self.update_slot(i,j)
