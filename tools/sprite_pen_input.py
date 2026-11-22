"""Read Windows Ink pen messages while leaving normal Tk mouse promotion intact."""
import ctypes as C
from ctypes import wintypes as W
from collections import deque
import sys


class PointerInfo(C.Structure):
    _fields_ = [('kind', W.DWORD), ('id', W.DWORD), ('frame', W.DWORD),
                ('flags', W.DWORD), ('device', W.HANDLE), ('window', W.HWND),
                ('pixel', W.POINT), ('himetric', W.POINT), ('raw_pixel', W.POINT),
                ('raw_himetric', W.POINT), ('time', W.DWORD), ('history', W.DWORD),
                ('input', W.LONG), ('keys', W.DWORD), ('performance', C.c_uint64),
                ('button', W.DWORD)]


class PenInfo(C.Structure):
    _fields_ = [('pointer', PointerInfo), ('flags', W.DWORD), ('mask', W.DWORD),
                ('pressure', W.DWORD), ('rotation', W.DWORD), ('tilt_x', W.LONG),
                ('tilt_y', W.LONG)]


class CanvasPen:
    def __init__(self, canvas):
        self.samples = deque(maxlen=256)
        if sys.platform != 'win32':
            return
        common = C.WinDLL('comctl32')
        read = C.windll.user32.GetPointerPenInfo
        read.argtypes = [W.UINT, C.POINTER(PenInfo)]
        word = C.c_size_t
        result = C.c_ssize_t
        callback = C.WINFUNCTYPE(result, W.HWND, W.UINT, word, result, word, word)
        common.DefSubclassProc.argtypes = [W.HWND, W.UINT, word, result]
        common.DefSubclassProc.restype = result
        common.SetWindowSubclass.argtypes = [W.HWND, callback, word, word]
        common.RemoveWindowSubclass.argtypes = [W.HWND, callback, word]

        @callback
        def process(hwnd, msg, wp, lp, ident, data):
            if msg in (0x245, 0x246):  # WM_POINTERUPDATE / DOWN
                pen = PenInfo()
                if read(wp & 0xffff, C.byref(pen)) and pen.mask & 1 and pen.pointer.flags & 4:
                    self.samples.append((pen.pointer.time, pen.pointer.pixel.x,
                                         pen.pointer.pixel.y, min(1., pen.pressure / 1024.)))
            elif msg == 0x82:  # WM_NCDESTROY
                common.RemoveWindowSubclass(hwnd, process, ident)
            return common.DefSubclassProc(hwnd, msg, wp, lp)

        self.callback = process
        common.SetWindowSubclass(canvas.winfo_id(), process, id(self), 0)

    def sample(self, event):
        for stamp, x, y, pressure in reversed(self.samples):
            if ((event.time - stamp) & 0xffffffff) <= 40 and abs(event.x_root-x)<=2 and abs(event.y_root-y)<=2:
                return pressure
        return 1.
