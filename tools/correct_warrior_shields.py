"""Reviewed shield templates propagated within matching animation directions.
Writes a separate editor project; original pixels and user masks stay unchanged.
"""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from sprite_material import Project
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'UserData/Materials/Warrior'
p=Project.load(OUT/'warrior.smproj')
seeds={r['index']:[tuple(v) for v in r['polygon']] for r in json.loads((OUT/'samples/round-shield/recipe.json').read_text())['samples']}
seeds.update({
2:[(7,19),(10,22),(11,36),(10,50),(7,52),(3,39),(3,27)],
7:[(23,30),(26,33),(26,48),(23,54),(21,48),(20,39)],
32:[(20,22),(24,24),(26,34),(25,45),(21,49),(17,43),(15,31)],
33:[(19,23),(25,26),(30,42),(29,52),(24,57),(16,53),(9,43),(7,33),(11,27)],
34:[(15,29),(20,30),(23,38),(22,51),(17,60),(12,57),(6,48),(4,38),(8,32)],
48:[(26,10),(33,11),(37,18),(37,30),(33,40),(27,43),(21,38),(18,29),(18,20),(21,14)],
49:[(26,9),(33,9),(38,14),(39,23),(36,34),(29,40),(23,39),(20,33),(19,23),(20,14)],
50:[(12,7),(15,9),(15,21),(14,32),(12,37),(9,30),(9,17)],
55:[(29,7),(32,9),(33,21),(31,33),(28,38),(26,32),(25,20),(26,11)],
72:[(26,10),(30,13),(31,23),(29,35),(27,39),(24,31),(23,21),(23,15)],
73:[(17,8),(24,8),(28,12),(28,22),(24,32),(18,38),(13,39),(9,34),(8,27),(11,16)],
74:[(19,10),(27,11),(29,16),(26,25),(18,34),(9,38),(3,36),(0,31),(3,23),(11,15)],
75:[(16,13),(22,14),(23,18),(18,25),(8,33),(2,34),(0,30),(3,23),(9,17)],
171:[(17,17),(23,17),(28,23),(30,33),(29,39),(24,43),(17,40),(12,35),(10,28),(12,21)],
172:[(20,17),(25,18),(30,27),(34,37),(33,42),(27,45),(19,41),(11,34),(6,26),(9,21)],
173:[(16,14),(20,17),(24,29),(28,40),(25,47),(20,43),(16,33),(12,20)],
178:[(34,18),(37,21),(37,32),(34,40),(32,36),(31,26)],
203:[(18,21),(24,22),(32,28),(35,33),(35,37),(30,40),(20,38),(15,35),(17,30),(13,26)],
204:[(21,20),(28,23),(36,31),(39,37),(36,43),(29,43),(18,38),(10,31),(7,26),(12,22)],
205:[(24,17),(29,22),(35,34),(38,43),(34,48),(29,43),(24,33),(20,23)],
210:[(29,13),(32,16),(34,25),(32,34),(29,31),(28,22)],
219:[(18,17),(24,18),(29,24),(32,33),(30,40),(25,43),(18,40),(13,34),(11,27),(13,21)],
220:[(27,17),(33,20),(39,30),(42,39),(38,44),(30,44),(21,37),(14,28),(16,22)],
221:[(15,12),(20,17),(24,29),(28,40),(25,46),(20,41),(15,30),(10,18)],
226:[(27,18),(29,22),(29,32),(27,39),(25,34),(25,24)],
120:[(26,18),(31,22),(33,32),(31,39),(27,42),(22,37),(19,29),(21,22)],
121:[(28,18),(35,21),(41,31),(44,40),(40,46),(33,44),(23,37),(16,28),(19,22)],
122:[(13,14),(18,17),(24,31),(28,40),(25,46),(19,41),(12,28),(8,19)],
127:[(34,18),(35,23),(35,31),(33,36),(33,24)],
128:[(20,17),(27,19),(33,26),(36,34),(33,39),(26,40),(16,35),(10,28),(12,22)],
129:[(22,16),(29,20),(38,31),(42,39),(38,45),(29,42),(17,33),(6,23),(10,18)],
130:[(16,11),(20,13),(23,24),(22,35),(18,42),(14,36),(10,25),(9,17)],
135:[(33,21),(36,25),(36,33),(34,39),(32,31)],
136:[(21,12),(29,16),(36,26),(38,33),(34,39),(25,37),(14,29),(9,21),(13,15)],
137:[(18,12),(26,15),(35,27),(39,36),(34,42),(24,38),(12,28),(4,19),(8,14)],
138:[(20,9),(23,11),(22,21),(18,31),(14,36),(11,32),(12,21),(15,13)],
143:[(28,15),(31,18),(35,29),(35,34),(32,31),(29,24)],
144:[(13,9),(20,13),(29,25),(34,31),(32,35),(23,34),(13,29),(4,22),(2,18),(7,12)],
145:[(23,8),(29,10),(32,20),(32,30),(28,36),(22,37),(16,32),(14,24),(14,16),(18,11)],
146:[(30,10),(32,12),(28,20),(20,29),(14,32),(17,24),(23,15)],
151:[(17,12),(25,16),(35,25),(40,32),(38,36),(30,33),(20,27),(16,20)],
152:[(13,8),(20,10),(27,18),(30,24),(27,28),(18,27),(8,23),(1,18),(2,13),(7,10)],
153:[(25,7),(32,9),(31,15),(25,22),(18,27),(14,27),(15,20),(19,12)],
154:[(41,4),(43,6),(38,10),(29,15),(17,20),(15,18),(23,11),(33,6)],
159:[(12,8),(21,11),(32,17),(39,23),(39,26),(30,26),(19,22),(12,17),(10,12)],
160:[(12,5),(19,6),(26,12),(29,18),(26,20),(17,20),(8,17),(3,13),(4,9)],
161:[(29,5),(39,5),(40,8),(31,13),(21,16),(16,15),(18,10)],
162:[(34,4),(42,5),(46,9),(42,11),(29,13),(15,13),(8,11),(15,7),(25,5)],
167:[(12,6),(22,10),(33,15),(38,18),(37,21),(28,21),(18,17),(10,12)],
168:[(15,1),(23,2),(29,5),(30,7),(22,7),(11,5),(6,3)],
169:[(10,0),(20,1),(32,5),(39,9),(38,13),(29,13),(16,10),(7,7),(5,3)],
170:[(7,0),(15,2),(23,8),(27,13),(26,17),(19,18),(10,15),(2,10),(0,5),(2,2)],
})
seeds.update({
8:[(12,13),(17,13),(21,17),(22,27),(20,39),(16,45),(11,44),(6,36),(3,25),(3,18),(7,14)],
16:[(13,17),(18,17),(22,22),(23,33),(21,44),(17,49),(12,47),(8,40),(5,29),(6,21)],
24:[(14,28),(19,29),(23,36),(24,48),(23,60),(20,66),(16,64),(12,56),(10,44),(10,34)],
40:[(20,17),(24,19),(27,25),(27,34),(25,42),(22,43),(19,37),(18,27)],
252:[(20,23),(27,24),(33,29),(36,36),(34,40),(27,42),(17,40),(10,36),(10,31),(14,26)],
260:[(20,22),(27,24),(33,30),(36,36),(35,40),(28,42),(18,40),(11,36),(10,31),(14,26)],
})
# Template candidates stay within their action family and viewing direction.
def family(i):
 if i<48:return 0
 if i<72:return 48
 if i<112:return 72
 if i<120:return 48
 if i<171:return 120
 if i<203:return 171
 if i<219:return 203
 return 219
starts={0:0,48:48,72:72,120:120,171:171,203:203,219:219}
def direction(i):return (i-(112 if 112<=i<120 else starts[family(i)]))%8

def match(i,candidates):
 target=np.asarray(p.frames[i].image,dtype=np.float32)
 best=None
 for seed in candidates:
  f=p.frames[seed];m=Image.new('L',f.image.size);ImageDraw.Draw(m).polygon(seeds[seed],fill=255)
  a=np.asarray(f.image,dtype=np.float32);yy,xx=np.where((np.asarray(m)>0)&(a[:,:,3]>0))
  if len(xx)<3:continue
  ref=a[yy,xx,:3];h,w=target.shape[:2]
  for dy in range(-int(yy.min()),h-int(yy.max())):
   for dx in range(-int(xx.min()),w-int(xx.max())):
    vals=target[yy+dy,xx+dx]
    score=float(np.mean((vals[:,:3]-ref)**2)+np.mean(vals[:,3]<1)*50000)
    if best is None or score<best[0]:best=(score,seed,dx,dy)
 if best is None:return None,None
 _,seed,dx,dy=best
 return [(x+dx,y+dy) for x,y in seeds[seed]],best

records=[]
for i,f in enumerate(p.frames):
 if i>=283 or i in (112,119):continue
 fam=family(i);d=direction(i)
 if fam in (171,203,219) and d==7:continue
 if i in seeds:poly=seeds[i];fit=(0,i,0,0)
 elif fam==120:continue # Explicit death silhouettes above; back faces stay unchanged.
 else:
  candidates=[s for s in seeds if family(s)==fam and direction(s)==d]
  if not candidates:continue
  poly,fit=match(i,candidates)
  if poly is None:continue
 mask=Image.new('L',f.image.size);ImageDraw.Draw(mask).polygon(poly,fill=255)
 m=(np.asarray(mask)>0)&(np.asarray(f.image)[:,:,3]>0)
 if m.sum()<3:
  print('Empty silhouette:',i);continue
 yy,xx=np.indices(m.shape);pts=np.column_stack((xx[m],yy[m])).astype(float)
 center=pts.mean(axis=0);_,axes=np.linalg.eigh(np.cov(pts.T));radii=np.max(np.abs((pts-center)@axes),axis=0)+.7
 uv=np.stack((xx-center[0],yy-center[1]),axis=2)@axes
 values=np.rint(80+145*np.maximum(0,1-np.sum((uv/radii)**2,axis=2))).astype('uint8')
 f.correction=Image.fromarray(values);f.correction_mask=Image.fromarray(m.astype('uint8')*255)
 records.append(dict(index=i,name=f.name,polygon=poly,template=int(fit[1]),fit_error=float(fit[0]),pixels=int(m.sum())))
 print(f'Shield {i+1:03d}/326',flush=True)
p.name='Warrior - rounded shields'
p.save(OUT/'warrior-rounded-shields.smproj')
(OUT/'shield-review/batch-recipe.json').write_text(json.dumps(dict(method='silhouette PCA paraboloid',base=80,bulge=145,excluded_parts=list(range(283,326)),frames=records),indent=2))
# Full audit pages: original with contour beside corrected height.
for start in range(0,len(records),32):
 board=Image.new('RGB',(1280,960),(40,44,50));draw=ImageDraw.Draw(board)
 for k,r in enumerate(records[start:start+32]):
  f=p.frames[r['index']];g=p.height(f);height=Image.merge('RGBA',(g,g,g,f.image.getchannel('A')))
  x=(k%8)*160;y=(k//8)*240
  for col,im in enumerate([f.image,height]):
   im=im.copy();im.thumbnail((76,210));im=im.resize((im.width*2,im.height*2),Image.Resampling.NEAREST) if im.width*2<=76 and im.height*2<=210 else im
   board.paste(im,(x+col*80,y+20),im)
  draw.text((x,y),str(r['index']),fill='white')
 board.save(OUT/f'shield-review/batch-{start:03d}.png')
print('Corrected:',len(records),'Excluded parts:',43)
