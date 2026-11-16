"""Machine-readable, flushed build progress; counts refer to the named stage."""
import json,time,os
PREFIX='@PROGRESS '
def progress(stage,current=None,total=None,item=''):
 if os.environ.get('MYTH_BUILD_PROGRESS')!='1':return
 print(PREFIX+json.dumps(dict(stage=stage,current=current,total=total,item=str(item)),ensure_ascii=True),flush=True)
def tracked(items,stage):
 items=list(items);total=len(items);last=0
 progress(stage,0,total)
 for i,item in enumerate(items):
  yield item
  now=time.monotonic()
  if now-last>=0.25 or i+1==total:
   progress(stage,i+1,total);last=now
