"""Sample-only, single-material shield reconstruction from reviewed silhouettes.

Unlike luminance conversion, original paint colors do not influence shield height.
These polygons are hand-reviewed examples, not automatic segmentation of all frames.
"""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from sprite_material import Project

ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'UserData/Materials/Warrior/samples/round-shield'
out.mkdir(parents=True,exist_ok=True)
project=Project.load(ROOT/'UserData/Materials/Warrior/warrior.smproj')
regions={
 0:[(20,26),(24,27),(27,31),(29,37),(30,45),(29,52),(27,57),(24,59),(20,57),(15,53),(11,47),(9,41),(8,37),(10,33),(15,29)],
 1:[(17,23),(20,25),(22,29),(25,35),(27,42),(28,47),(27,52),(24,55),(21,56),(17,53),(12,48),(8,42),(5,36),(3,31),(4,28),(8,26),(12,24)],
 9:[(16,11),(19,12),(21,15),(22,20),(21,26),(20,32),(18,38),(15,42),(11,45),(7,45),(4,42),(2,38),(1,32),(1,26),(1,21),(2,17),(5,14),(10,12)]
}
font=ImageFont.truetype('C:/Windows/Fonts/meiryo.ttc',18)
board=Image.new('RGB',(1120,990),(35,39,46));draw=ImageDraw.Draw(board)
for j,title in enumerate(['元画像','曲面から作ったハイト','ノーマル','単一色の盾・照明例']):
 draw.text((j*280+12,12),title,font=font,fill='white')
records=[]
for row,(index,poly) in enumerate(regions.items()):
 f=project.frames[index];before=np.asarray(project.height(f));mask=Image.new('L',f.image.size)
 ImageDraw.Draw(mask).polygon(poly,fill=255)
 m=(np.asarray(mask)>0)&(np.asarray(f.image)[:,:,3]>0)
 # Fit a tilted ellipse in image space; ignore paint, palette and baked light.
 yy,xx=np.indices(m.shape);points=np.column_stack((xx[m],yy[m])).astype(float)
 center=points.mean(axis=0);_,axes=np.linalg.eigh(np.cov(points.T))
 uv=(np.stack((xx-center[0],yy-center[1]),axis=2)@axes)
 radii=np.max(np.abs((points-center)@axes),axis=0)+.7
 radius2=np.sum((uv/radii)**2,axis=2)
 dome=np.maximum(0,1-radius2)
 # Shallow convex paraboloid, with constant rim height. No painted stripe.
 values=80+145*dome
 result=before.copy();result[m]=np.rint(values[m]).astype(np.uint8)
 height=Image.fromarray(result);alpha=f.image.getchannel('A')
 rgba=Image.merge('RGBA',(height,height,height,alpha));normal=project.normal(f,height)
 # Preview material under one fixed directional light; this is not a VR capture.
 n=np.asarray(normal)[:,:,:3].astype(float)/255*2-1
 n/=np.maximum(np.linalg.norm(n,axis=2,keepdims=True),1e-6)
 light=np.array([-.55,.65,.65]);light/=np.linalg.norm(light)
 diffuse=np.maximum(0,n@light)
 shade=.23+.77*diffuse
 original=np.asarray(f.image).copy();base=np.array([135.,158.,190.])
 original[m,:3]=np.rint(shade[m,None]*base).astype(np.uint8)
 preview=Image.fromarray(original)
 assert np.array_equal(result[~m],before[~m])
 assert np.array_equal(original[~m],np.asarray(f.image)[~m])
 for name,im in [('mask',mask),('height',rgba),('normal',normal),('material-preview',preview)]:im.save(out/f'{index:03d}-{name}.png')
 for col,im in enumerate([f.image,rgba,normal,preview]):
  scale=min(260/im.width,270/im.height);im=im.resize((round(im.width*scale),round(im.height*scale)),Image.Resampling.NEAREST)
  board.paste(im,(col*280+(280-im.width)//2,55+row*305),im)
 draw.text((12,55+row*305+272),f.name,font=font,fill=(210,215,220))
 records.append(dict(index=index,polygon=poly,center=center.tolist(),radii=radii.tolist(),identity=f.identity))
board.save(out/'comparison.png')
(out/'recipe.json').write_text(json.dumps(dict(method='silhouette-fit shallow convex paraboloid',base_height=80,bulge=145,samples=records),indent=2),encoding='utf8')
print(out/'comparison.png')
