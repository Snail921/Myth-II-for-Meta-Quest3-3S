"""Offline map dependency inventory using the pinned MIT mythextract reader."""
import sys,json,struct,shutil,contextlib,io
from pathlib import Path
from types import SimpleNamespace
ROOT=Path(__file__).resolve().parents[1]
# The pinned reader's optional stats UI uses Python 3.12 f-string syntax.
# Keep a compatibility copy; original sources and their license stay untouched.
reader=ROOT/'build/map-readers';reader.mkdir(exist_ok=True)
for src in (ROOT/'research/references/mythextract/scripts').glob('*.py'):
 text=src.read_text(encoding='utf8')
 if src.name=='mons_tag.py':text=text.replace('import mons2stats','') # CLI stats UI is never used by the dependency reader
 (reader/src.name).write_text(text,encoding='utf8')
sys.path.insert(0,str(reader))
import tag2local,loadtags
from prepare_map_fixture import parse_mesh,schemas

from tag_sources import inventory

def raw_tag(tags,kind,name):
 a,t=tags[kind,name]
 with (ROOT/a).open('rb') as f:f.seek(t['offset']);return f.read(t['size'])

def dependencies(tags,mapid):
 fetched={};missing=[]
 def fetch(_tags,_data,typ,name):
  if (typ,name) not in tags:
   if name not in ('\xff'*4,'\x00'*4,''):missing.append((typ,name))
   return None,None,None
  raw=raw_tag(tags,typ,name);a,t=tags[typ,name];fetched[typ,name]=raw
  if typ in ('.256','soun','amso','stli','text','dmap'):return None,None,None
  header=SimpleNamespace(tag_type=typ,tag_id=name,name=t['name'],version=0,signature='mth2')
  return a,header,bytes(60)+b'mth2'+raw
 original=loadtags.get_tag_info;loadtags.get_tag_info=fetch
 try:
  g=tag2local.TagDataGenerator(2,{}, {},[]);g.FETCHED={}
  with contextlib.redirect_stdout(io.StringIO()):
   list(g.get_tag_data('mesh',mapid.encode('latin1')))
   # The pinned reader does not follow MEEF's collection reference. Ring
   # effects must resolve their own texture on every map, not rely on the
   # historical shared-effects fallback list.
   for (typ,name),data in list(fetched.items()):
    if typ=='meef' and len(data)==36:
     list(g.get_tag_data('core',data[16:20]))
 finally:loadtags.get_tag_info=original
 return fetched,missing
if __name__=='__main__':
 tags=inventory()
 for mapid in ('l1  ','cr01'):
  m=parse_mesh(raw_tag(tags,'mesh',mapid),schemas());deps,missing=dependencies(tags,mapid)
  cols=[(k,len(v)) for k,v in deps.items() if k[0]=='.256']
  print(mapid,m['width'],m['height'],m['header']['landscape_collection_tag'],len(deps),ascii(cols),'missing',ascii(missing))


