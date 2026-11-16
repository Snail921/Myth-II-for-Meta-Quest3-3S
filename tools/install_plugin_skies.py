import json,shutil,hashlib,argparse,subprocess,shlex
from pathlib import Path
from PIL import Image,ImageDraw
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--deploy',action='store_true');a=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'Skyboxes';plan=json.loads((root/'build/plugin-skybox-plan.json').read_text(encoding='utf8'));results={r['id']:r for r in json.loads((root/'build/plugin-skybox-generation-results.json').read_text(encoding='utf-8-sig'))}
records=[];images=[];files={};missing=[]
for j in plan['jobs']:
 if j['id'] not in results:missing.append(j['id']);continue
 r=results[j['id']];src=Path(r['path']);digest=hashlib.sha256(src.read_bytes()).hexdigest()
 with Image.open(src) as im:
  w,h=im.size;assert w==2*h and w<=4096 and h<=2048
  pixels=np.asarray(im.convert('RGB'),dtype=np.float32);seam=float(np.abs(pixels[:,0]-pixels[:,-1]).mean())
  thumb=im.convert('RGB');thumb.thumbnail((320,160));images.append((j['levels'][0]['name'],thumb))
 for l in j['levels']:
  n=l['name'];assert not any(c in n for c in '/\\\0')
  names=list(dict.fromkeys([n,n.strip()]))
  for name in names:
   target=out/(name+'.png')
   if target.exists() and hashlib.sha256(target.read_bytes()).hexdigest()!=digest:raise ValueError('Existing asset conflict: '+str(target))
   shutil.copy2(src,target);files[target.name]=digest
  meta=dict(level=n,map_key=l['key'],plugin=l['archive'],image=n+'.png',aliases=names,projection='equirectangular',size=[w,h],sha256=digest,design=j['theme'],prompt=j['prompt'],generation_tool='built-in image_gen',journal=l['journal'],lighting=l['lighting'],weather=l['weather'],seam_mean_absolute_rgb=round(seam,3),status='created',limitations='Artistic atmosphere, not a geometric continuation of terrain; headset review pending. Skybox placement does not enable an otherwise unsupported plugin level.')
  (out/(n+'.skybox.json')).write_text(json.dumps(meta,ensure_ascii=False,indent=2)+'\n',encoding='utf8');records.append(meta)
manifest=dict(total_level_entries=plan['entries'],design_count=len(results),levels=records,excluded=plan['excluded'],missing_designs=missing,quest_directory='/sdcard/Android/data/org.mythvr.standalone/files/Skyboxes/')
if a.deploy:
 assert not missing
 adb='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe';remote=manifest['quest_directory'];assert len(records)+len(plan['excluded'])==plan['entries']
 for name,digest in files.items():
  subprocess.run([adb,'-s','340YC10G8T047C','push',str(out/name),remote+name],check=True,stdout=subprocess.DEVNULL)
  got=subprocess.check_output([adb,'-s','340YC10G8T047C','shell','sha256sum '+shlex.quote(remote+name)],encoding='utf8').split()[0]
  assert got==digest,name
 for row in records:row['status']='deployed_sha256_verified'
 manifest['deployed_files']=len(files)
(out/'plugin-skyboxes.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
for page,start in enumerate(range(0,len(images),12)):
 sheet=Image.new('RGB',(960,4*190),(25,25,25));draw=ImageDraw.Draw(sheet)
 for k,(name,im) in enumerate(images[start:start+12]):
  px=(k%3)*320;py=(k//3)*190;sheet.paste(im,(px,py));draw.text((px+4,py+164),name,fill='white')
 sheet.save(root/f'build/plugin-skybox-preview-{page+1}.jpg')
print(json.dumps(dict(designs=len(results),levels=len(records),missing=len(missing),deployed=a.deploy,largest_seam=max(r['seam_mean_absolute_rgb'] for r in records))))

