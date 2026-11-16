"""Estimate original status text fit at 120% panel width; does not edit the app."""
from pathlib import Path
import sys,re,struct,json,math
from level_assets import inventory,raw_tag
root=Path(__file__).resolve().parents[1]
tags=inventory()
font=raw_tag(tags,'font','co1b');pos=64;advance=[14]*256
for _ in range(struct.unpack_from('>I',font,12)[0]):
 c,a,w,h,x,y=struct.unpack_from('>4H2h',font,pos);advance[c]=max(1,a);pos+=16+w*h+(w*h&1)
def strings(key):return [s for s in re.split(b'[\r\n\0]+',raw_tag(tags,'stli',key)) if s]
def layout(title,name,description,scale,stock,fixed=False,width_extra=.05):
 pixel=.00034*scale;pitch=.0085*(1 if fixed else scale);rows=[];line=b'';pen=0.;y=.036
 def width():return (.140 if stock and y>.024 else .174)+width_extra
 def newline():
  nonlocal line,pen,y
  rows.append(line);line=b'';pen=0.;y-=pitch
 for section in [title,name,description]:
  if rows or line:newline()
  for token in re.findall(rb'\S+| +',section):
   tw=sum(advance[c]*pixel for c in token)
   if token.isspace():
    if not pen:continue
    if pen+tw>width():newline();continue
   elif pen and pen+tw>width():newline()
   for c in token:
    cw=advance[c]*pixel
    if pen+cw>width():newline()
    line+=bytes([c]);pen+=cw
 rows.append(line)
 fits=(len(rows)<=4 if fixed else y-21*pixel>=.003125)
 return fits,len(rows),[s.decode('mac_roman') for s in rows]
classes=[('Warrior','wasp','wana','wafl',False),('Bowman','arsp','arna','arfl',True),('Dwarf','dwsp','dwna','dwfl',True),('Journeyman','josq','jonb','jofm',True),('Berserk','besp','bena','befl',False),('Warlock','maud','ma01','ma02',False)]
result=[]
for label,spell,names,flavors,stock in classes:
 title=strings(spell)[0];n=strings(names)[:128];f=strings(flavors);cases=[]
 for i,name in enumerate(n):
  desc=f[i%len(f)];threshold=next(p for p in range(100,29,-1) if layout(title,name,desc,p/100,stock)[0]);fixed=next(p for p in range(100,19,-1) if layout(title,name,desc,p/100,stock,True)[0])
  cases.append(dict(name=name.decode('mac_roman'),description=desc.decode('mac_roman'),characters=len(desc),max_percent=threshold,fixed_four_rows_percent=fixed,lines_at_limit=layout(title,name,desc,threshold/100,stock)[1],lines_at_100=layout(title,name,desc,1,stock)[1],fits_at_80=layout(title,name,desc,.8,stock)[0],fits_at_75=layout(title,name,desc,.75,stock)[0]))
 result.append(dict(type=label,names=len(n),descriptions=len(f),minimum_percent=min(c['max_percent'] for c in cases),maximum_percent=max(c['max_percent'] for c in cases),fixed_four_rows=min(c['fixed_four_rows_percent'] for c in cases),sample=cases[0],all_cases=cases))
report=root/'research/reports/info-panel-width-estimate.json';report.write_text(json.dumps({'assumptions':{'width_m':.30,'height_m':.04,'text_width_m':.224,'inventory_reserved_text_width_m':.190,'font_pixel_m':.00034,'line_pitch_m':.0085,'top_m':.036,'bottom_margin_m':.003125,'scales':'all text plus line pitch; portrait and inventory unchanged'},'results':result},ensure_ascii=False,indent=2),encoding='utf-8')
for r in result:
 print(r['type'],r['names'],'names',r['minimum_percent'],r['maximum_percent'],'fixed4',r['fixed_four_rows'],'example',r['sample']['characters'],r['sample']['max_percent'],'lines100',r['sample']['lines_at_100'],'fits80',all(c['fits_at_80'] for c in r['all_cases']),'fits75',all(c['fits_at_75'] for c in r['all_cases']))
