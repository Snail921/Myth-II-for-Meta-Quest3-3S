"""Package separately; canonical 1.8.1 APK is read-only input."""
from pathlib import Path
import subprocess,sys,os
root=Path(__file__).resolve().parents[1];os.chdir(root)
work=Path(os.environ['TEMP'])/'myth-185-package';work.mkdir(exist_ok=True)
out=Path('build/integration-185');pairs=['classes2.dex='+str(out/'classes/classes.dex'),'lib/arm64-v8a/libmyth_capture.so='+str(out/'libmyth_capture.so'),'lib/arm64-v8a/libmythvr.so='+str(out/'libmythvr.so'),'assets/standalone/mythinput.exe='+str(out/'mythinput.exe'),'assets/standalone/mythinput-v7.dll='+str(out/'mythinput-v7.dll')]
for p in (out/'engine185').rglob('*'):
 if p.is_file():pairs.append('assets/engine185/'+p.relative_to(out/'engine185').as_posix()+'='+str(p))
subprocess.run([sys.executable,'tools/patch_apk_fast.py','build/myth-standalone.apk',str(work/'unsigned.apk'),*pairs],check=True)
bt=Path('C:/Users/gen/AppData/Local/Android/Sdk/build-tools/30.0.2');env=dict(os.environ,JAVA_HOME=str(root/'tools/runtime/jdk-21.0.13+9'))
subprocess.run([str(bt/'zipalign.exe'),'-f','4',str(work/'unsigned.apk'),str(work/'aligned.apk')],check=True)
subprocess.run([str(bt/'apksigner.bat'),'sign','--ks','build/vr/debug.keystore','--ks-pass','pass:android','--out',str(work/'signed.apk'),str(work/'aligned.apk')],env=env,check=True)
subprocess.run([str(bt/'apksigner.bat'),'verify','--verbose',str(work/'signed.apk')],env=env,check=True)
import shutil,hashlib,json
final=Path('build/myth-standalone-185-preview.apk');shutil.copyfile(work/'signed.apk',final)
report={'apk':str(final),'sha256':hashlib.sha256(final.read_bytes()).hexdigest(),'canonical_181_sha256':hashlib.sha256(Path('build/myth-standalone.apk').read_bytes()).hexdigest(),'stage':'limited VR integration preview','signature_verified':True,'full_185_support':False,'game_directory':'/storage/emulated/0/Download/MythII-Standalone185','supported_commands':[1,2,3,4,8,9,10,11,13,14,15],'op4_limitation':'friendly selection only; attacks rejected','unsupported':['combat commands','pickup','groups','special abilities','script camera synchronization','audio listener/range synchronization','live FX','live terrain changes','selection information']}
Path('research/reports/version-185-apk.json').write_text(json.dumps(report,indent=2),encoding='utf8');print(json.dumps(report))
