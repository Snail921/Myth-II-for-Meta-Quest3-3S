"""Export normal-only libraries; restore runtime packs using user-owned source images."""
import argparse, hashlib, json, struct, tempfile, zipfile
from pathlib import Path

LIMIT=64*1024*1024

def identity(w,h,pixels):
    return hashlib.sha256(struct.pack('<II',w,h)+pixels).hexdigest()

def frames(path):
    with Path(path).open('rb') as f:
        if f.read(8)!=b'MVRNORM1':raise ValueError('Not a normal pack: '+str(path))
        count,=struct.unpack('<I',f.read(4))
        if not 0<count<=10000:raise ValueError('Frame count')
        for _ in range(count):
            w,h=struct.unpack('<II',f.read(8));size=w*h*4
            if not (0<w<=2048 and 0<h<=2048):raise ValueError('Dimensions')
            source=f.read(size);normal=f.read(size)
            if len(source)!=size or len(normal)!=size:raise ValueError('Truncated pack')
            yield w,h,source,normal
        if f.read(1):raise ValueError('Trailing bytes')

def export_library(folder,output):
    packs=[];seen=set();paths=sorted(Path(folder).glob('*.mvrnm'))
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for i,p in enumerate(paths):
            rows=[]
            for w,h,source,normal in frames(p):
                key=hashlib.sha256(normal).hexdigest()
                if key not in seen:z.writestr('normals/'+key+'.rgba',normal);seen.add(key)
                rows.append(dict(w=w,h=h,source=identity(w,h,source),normal=key))
            packs.append(dict(name=p.name,frames=rows))
            print(f'Export {i+1}/{len(paths)}: {p.name}',flush=True)
        z.writestr('library.json',json.dumps(dict(version=1,packs=packs),ensure_ascii=False))
    return len(packs)

class FolderLibrary:
    def __init__(self,path):self.path=Path(path)
    def __enter__(self):return self
    def __exit__(self,*args):pass
    def read(self,name):return (self.path/name).read_bytes()
    def getinfo(self,name):
        from types import SimpleNamespace
        return SimpleNamespace(file_size=(self.path/name).stat().st_size)

def restore_library(library,collections,output):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    with (FolderLibrary(library) if Path(library).is_dir() else zipfile.ZipFile(library)) as z, tempfile.TemporaryDirectory(prefix='myth-normal-') as scratch:
        if z.getinfo('library.json').file_size>32*1024*1024:raise ValueError('Manifest too large')
        data=json.loads(z.read('library.json'))
        if data['version']!=1:raise ValueError('Unsupported library version')
        needed={};names=set()
        for pack in data['packs']:
            name=pack['name']
            if Path(name).name!=name or '/' in name or '\\' in name or ':' in name or not name.endswith('.mvrnm') or name.casefold() in names:raise ValueError('Invalid pack name')
            names.add(name.casefold())
            if not 0<len(pack['frames'])<=10000:raise ValueError('Frame count')
            for row in pack['frames']:
                for field in ['source','normal']:
                    if len(row[field])!=64 or any(c not in '0123456789abcdef' for c in row[field]):raise ValueError('Invalid hash')
                if not (0<row['w']<=2048 and 0<row['h']<=2048):raise ValueError('Dimensions')
                needed[row['source']]=(row['w'],row['h'])
        found=set();dimensions=set(needed.values())
        def accept(w,h,rgba):
            if (w,h) not in dimensions:return
            key=identity(w,h,rgba)
            if key in needed and key not in found:
                Path(scratch,key).write_bytes(rgba);found.add(key)
        files=sorted(p for p in Path(collections).rglob('*') if p.is_file() and p.suffix.lower() in ('.bin','.collection','.256','.mvrnm','.png'))
        for i,p in enumerate(files):
            try:
                if p.suffix.lower() in ('.bin','.collection','.256'):
                    from prepare_vr_assets import decode
                    images,_,_=decode(p.read_bytes())
                    for w,h,pixels in images:accept(w,h,bytes(pixels))
                elif p.suffix.lower()=='.mvrnm':
                    for w,h,source,_ in frames(p):accept(w,h,source)
                else:
                    from PIL import Image
                    with Image.open(p) as im:accept(im.width,im.height,im.convert('RGBA').tobytes())
            except (ValueError,IndexError,struct.error,KeyError,OverflowError) as exc:
                print('Skip non-collection:',p.name,type(exc).__name__,flush=True)
            print(f'Scan {i+1}/{len(files)}, matched {len(found)}/{len(needed)}: {p.name}',flush=True)
        written=0;matched=0;total=0
        for pack in data['packs']:
            rows=[row for row in pack['frames'] if row['source'] in found];total+=len(pack['frames'])
            if not rows:continue
            size=12+sum(8+row['w']*row['h']*8 for row in rows)
            if size>LIMIT:raise ValueError('Pack exceeds 64 MiB: '+pack['name'])
            target=output/pack['name'];temp=target.with_suffix('.tmp')
            try:
                with temp.open('wb') as f:
                    f.write(b'MVRNORM1'+struct.pack('<I',len(rows)))
                    for row in rows:
                        size=row['w']*row['h']*4;member='normals/'+row['normal']+'.rgba'
                        if z.getinfo(member).file_size!=size:raise ValueError('Normal size mismatch')
                        normal=z.read(member)
                        if hashlib.sha256(normal).hexdigest()!=row['normal']:raise ValueError('Normal hash mismatch')
                        f.write(struct.pack('<II',row['w'],row['h']));f.write(Path(scratch,row['source']).read_bytes());f.write(normal)
                temp.replace(target)
            finally:
                if temp.exists():temp.unlink()
            written+=1;matched+=len(rows)
            print(f'Restored {written}: {pack["name"]} ({len(rows)}/{len(pack["frames"])})',flush=True)
        print(f'Done: {written} packs, {matched}/{total} frames. Unmatched images were skipped.',flush=True)
        if not written:raise ValueError('No matching source images. Select the converted collections folder.')
        return written,matched,total

def gui():
    import tkinter as tk
    from tkinter import filedialog,messagebox
    root=tk.Tk();root.withdraw()
    library=Path(__file__).resolve().parents[1]
    if not (library/'library.json').exists():
        library=filedialog.askopenfilename(title='Normal map library ZIP / ノーマルマップ集ZIP',filetypes=[('ZIP','*.zip')])
        if not library:return
    collections=filedialog.askdirectory(title='Your converted collections / 自分の変換済みコレクションのフォルダ')
    if not collections:return
    output=filedialog.askdirectory(title='Output folder for Quest packs / Quest用パックの保存先')
    if not output:return
    root.destroy()
    written,matched,total=restore_library(library,collections,output)
    print('Transfer the output folder with Plugin Manager / 保存先をManagerからQuestへ転送してください。',flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    sub.add_parser('gui');e=sub.add_parser('export');e.add_argument('folder');e.add_argument('output')
    r=sub.add_parser('restore');r.add_argument('library');r.add_argument('collections');r.add_argument('output')
    a=p.parse_args()
    if a.command=='gui':gui()
    elif a.command=='export':export_library(a.folder,a.output)
    else:restore_library(a.library,a.collections,a.output)
if __name__=='__main__':main()
