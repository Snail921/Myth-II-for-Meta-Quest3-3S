"""Reproducible shield-paint interpolation samples, not a batch segmentation model."""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from sprite_material import Project

ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'UserData/Materials/Warrior/samples'
out.mkdir(parents=True,exist_ok=True)
project=Project.load(ROOT/'UserData/Materials/Warrior/warrior.smproj')
# Reviewed source-pixel regions. Outside these regions is never modified.
regions={
 0:[(20,26),(23,27),(25,33),(25,50),(24,56),(20,54),(16,50),(16,44),(19,36)],
 1:[(18,25),(20,29),(20,43),(19,51),(17,52),(12,45),(10,39),(10,33),(13,29)],
 9:[(17,12),(20,12),(19,19),(12,27),(10,34),(10,40),(4,40),(3,33),(3,26),(7,21),(13,17)]
}
font=ImageFont.truetype('C:/Windows/Fonts/meiryo.ttc',18)
board=Image.new('RGB',(1120,990),(35,39,46));draw=ImageDraw.Draw(board)
for j,title in enumerate(['元画像','元のハイト','模様を補間したハイト','補正後のノーマル']):draw.text((j*280+12,12),title,font=font,fill='white')
report=[]
for row,(index,poly) in enumerate(regions.items()):
 f=project.frames[index];h=project.height(f);a=np.asarray(h,dtype=np.float64)
 mask=Image.new('L',f.image.size);ImageDraw.Draw(mask).polygon(poly,fill=255)
 m=(np.asarray(mask)>0)&(np.asarray(f.image)[:,:,3]>0)
 # Harmonic interpolation: preserves surrounding boundary values without
 # introducing a constant fill or altering the source color image.
 result=a.copy();result[m]=np.mean(a[~m & (np.asarray(f.image)[:,:,3]>0)])
 for iteration in range(3000):
  p=np.pad(result,1,mode='edge');average=(p[:-2,1:-1]+p[2:,1:-1]+p[1:-1,:-2]+p[1:-1,2:])*.25
  delta=np.max(np.abs(result[m]-average[m]));result[m]=average[m]
  if delta<.001:break
 corrected=Image.fromarray(np.rint(result).astype(np.uint8))
 assert np.array_equal(np.asarray(corrected)[~m],np.asarray(h)[~m])
 def rgba(gray):return Image.merge('RGBA',(gray,gray,gray,f.image.getchannel('A')))
 normal=project.normal(f,corrected)
 for name,im in [('source',f.image),('before',rgba(h)),('height',rgba(corrected)),('normal',normal),('mask',mask)]:im.save(out/f'sample-{index:03d}-{name}.png')
 for col,im in enumerate([f.image,rgba(h),rgba(corrected),normal]):
  scale=min(260/im.width,270/im.height);im=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.NEAREST)
  x=col*280+(280-im.width)//2;y=55+row*305;board.paste(im,(x,y),im)
 draw.text((12,55+row*305+272),f'{f.name} · 補正 {int(m.sum())} px',font=font,fill=(210,215,220))
 report.append(dict(index=index,name=f.name,polygon=poly,modified_pixels=int(m.sum()),iterations=iteration+1,identity=f.identity))
board.save(out/'shield-height-comparison.png')
(out/'shield-height-sample.json').write_text(json.dumps(dict(method='harmonic interpolation inside reviewed shield paint polygons',normal_strength=project.strength,samples=report),ensure_ascii=False,indent=2),encoding='utf8')
print(out/'shield-height-comparison.png')
