"""Replace APK ZIP entries without recompressing unchanged runtime assets. Sign afterwards."""
import sys, zipfile, copy, shutil
from pathlib import Path
source, target, *pairs = sys.argv[1:]
removed = [p[9:] for p in pairs if p.startswith("--remove=")]
replacements = dict(p.split("=", 1) for p in pairs if not p.startswith("--remove="))
def omit(name): return name.startswith("META-INF/") or any(name==r or (r.endswith("/") and name.startswith(r)) for r in removed)
if source == target: raise ValueError("Separate output required")
# The standalone extension belongs in classes2.dex. Losing the base provider
# prevents Android from creating ANY activity, even when APK signing succeeds.
if "classes.dex" in replacements:
 with zipfile.ZipFile(source) as original:
  required=b"Landroidx/core/content/FileProvider;"
  if required in original.read("classes.dex") and required not in Path(replacements["classes.dex"]).read_bytes():
   raise ValueError("Base DEX would lose FileProvider; standalone code must replace classes2.dex")
with zipfile.ZipFile(source) as src, open(source, "rb") as raw, zipfile.ZipFile(target,"w",zipfile.ZIP_DEFLATED) as dst:
 entries=sorted(src.infolist(), key=lambda i:i.header_offset)
 for n,entry in enumerate(entries):
  if entry.filename in replacements or omit(entry.filename): continue
  end=entries[n+1].header_offset if n+1<len(entries) else src.start_dir
  raw.seek(entry.header_offset); info=copy.copy(entry); info.header_offset=dst.fp.tell()
  remaining=end-entry.header_offset
  while remaining:
   block=raw.read(min(remaining,1024*1024))
   if not block: raise EOFError()
   dst.fp.write(block); remaining-=len(block)
  dst.filelist.append(info); dst.NameToInfo[info.filename]=info
 dst.start_dir=dst.fp.tell(); dst._didModify=True
 for name,path in replacements.items():dst.write(path,name)
with zipfile.ZipFile(source) as src,zipfile.ZipFile(target) as dst:
 for name,path in replacements.items():
  from pathlib import Path
  assert dst.read(name)==Path(path).read_bytes(),name
 for info in src.infolist():
  if info.filename in replacements or omit(info.filename):continue
  other=dst.getinfo(info.filename)
  assert (other.CRC,other.file_size)==(info.CRC,info.file_size)
print("APK entries verified")
