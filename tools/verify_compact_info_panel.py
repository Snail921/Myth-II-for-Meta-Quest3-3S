"""Check native word wrapping against all monster text tables in installed assets."""
from pathlib import Path
import json,re,struct
from level_assets import inventory,raw_tag
import mons_tag
root=Path(__file__).resolve().parents[1]
tags=inventory()
font=(root/'build/vr/assets/info-font.rgba').read_bytes()
advance=struct.unpack_from('<256H',font,8+512*512*4)
pixel=.00034*.78

def strings(key):return [s for s in re.split(rb'[\r\n\0]+',raw_tag(tags,'stli',key)) if s]
def fits(text,width,stock):
 row=0;pen=0.
 for i,c in enumerate(text):
  if c in (10,13):
   if c==10 or i+1==len(text) or text[i+1]!=10:row+=1;pen=0.
   continue
  if c==32 and pen==0:continue
  limit=width-(.034 if stock and .036-row*.0085*.78>.024 else 0)
  if c!=32 and (i==0 or text[i-1] in (32,10)):
   j=i
   while j<len(text) and text[j] not in (32,10,13):j+=1
   word=sum(advance[k]*pixel for k in text[i:j])
   if pen and word<limit and pen+word>limit:row+=1;pen=0.
  if pen+advance[c]*pixel>limit:row+=1;pen=0.
  if row>=5:return False
  pen+=advance[c]*pixel
 return True
seen=set();results=[]
for kind,key in tags:
 if kind!='mons':continue
 m=mons_tag.parse_tag(2,bytes(64)+raw_tag(tags,kind,key))
 refs=tuple(getattr(m,f).decode('latin1') for f in ('spelling_string_list_tag','names_string_list_tag','flavor_string_list_tag'))
 if refs in seen or any(('stli',r) not in tags for r in refs):continue
 seen.add(refs);title=strings(refs[0])[0];cases=[]
 for name in strings(refs[1])[:128]:
  for desc in strings(refs[2]):
   text=b'\n'.join((title,name,desc))
   assert fits(text,.174,True),(key,name,desc)
   minimum=next(mm for mm in range(100,175) if fits(text,mm/1000,True))
   cases.append(minimum)
 results.append(dict(title=title.decode('mac_roman'),cases=len(cases),required_mm=max(cases,default=0)))
report={'font_scale':.78,'rows':5,'text_width_mm':174,'stock_top_rows_width_mm':140,'panel_width_mm':334,'previous_panel_width_mm':384,'center_offset_mm':17,'types':len(results),'cases':sum(r['cases'] for r in results),'maximum_required_mm':max(r['required_mm'] for r in results),'results':results,'scope':'Single units with referenced title/name/flavor tables; all candidate names x descriptions. Multiple-selection name lists remain subject to five-row limit.'}
(root/'research/reports/info-panel-compact-fit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
print('PASS',report['types'],'types',report['cases'],'cases; minimum worst-case text width',report['maximum_required_mm'],'mm')
assert abs((-.150-.017)+(.184-.017))<1e-8
assert .164 <= .184-32*(.040/64)+1e-8
print('PASS panel center and right ornament clearance')
