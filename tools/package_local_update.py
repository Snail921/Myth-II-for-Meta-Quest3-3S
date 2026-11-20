"""Package a locally tested extension update, preserving base runtime assets."""
from pathlib import Path
import subprocess,sys,os,zipfile,hashlib,json
base,output,*pairs=sys.argv[1:]
target=Path(output);unsigned=target.with_name(target.stem+'-unsigned.apk');aligned=target.with_name(target.stem+'-aligned.apk')
subprocess.run([sys.executable,'tools/patch_apk_fast.py',base,str(unsigned),*pairs],check=True)
bt=Path('C:/Users/gen/AppData/Local/Android/Sdk/build-tools/30.0.2')
env=dict(os.environ,JAVA_HOME=str(Path('tools/runtime/jdk-21.0.13+9').resolve()))
subprocess.run([str(bt/'zipalign.exe'),'-f','4',str(unsigned),str(aligned)],check=True)
subprocess.run([str(bt/'apksigner.bat'),'sign','--ks','build/vr/debug.keystore','--ks-pass','pass:android','--out',str(target),str(aligned)],env=env,check=True)
subprocess.run([str(bt/'apksigner.bat'),'verify','--verbose',str(target)],env=env,check=True)
with zipfile.ZipFile(target) as z:
 for pair in pairs:
  name,file=pair.split('=',1)
  assert hashlib.sha256(z.read(name)).digest()==hashlib.sha256(Path(file).read_bytes()).digest(),name
print(json.dumps({'apk':str(target),'sha256':hashlib.sha256(target.read_bytes()).hexdigest()}))
