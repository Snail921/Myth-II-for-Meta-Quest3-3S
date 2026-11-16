"""Package separately; canonical 1.8.1 APK is read-only input."""
from pathlib import Path
import subprocess,sys,os
root=Path(__file__).resolve().parents[1];os.chdir(root)
work=root/'build/package-185-full';work.mkdir(exist_ok=True)
out=Path('build/integration-185');pairs=['classes2.dex='+str(out/'classes/classes.dex'),'lib/arm64-v8a/libmyth_capture.so='+str(out/'libmyth_capture.so'),'lib/arm64-v8a/libmythvr.so='+str(out/'libmythvr.so'),'assets/standalone/mythinput.exe='+str(out/'mythinput.exe'),'assets/standalone/mythinput-v7.dll='+str(out/'mythinput-v7.dll')]
remove=['assets/levels/','assets/engine185/']+['assets/'+n for n in ('animated-models.pack','models.pack','particles.pack','reflection-water.pack','scene.pack','snapshot.bin','sprite-indices.pack','surface-effects.pack','connectors.pack','fog.pack','water-light.pack','info-font.rgba','info-decoration.pack','dialog-templates.pack','standalone/Logo.png','standalone/CD_Face.png')]
pairs+=['--remove='+n for n in remove]
empty=work/'catalog.tsv';empty.write_text('',encoding='utf8');pairs.append('assets/levels/catalog.tsv='+str(empty))
subprocess.run([sys.executable,'tools/patch_apk_fast.py','build/myth-standalone-185-candidate.apk',str(work/'unsigned.apk'),*pairs],check=True)
bt=Path('C:/Users/gen/AppData/Local/Android/Sdk/build-tools/30.0.2');env=dict(os.environ,JAVA_HOME=str(root/'tools/runtime/jdk-21.0.13+9'))
subprocess.run([str(bt/'zipalign.exe'),'-f','4',str(work/'unsigned.apk'),str(work/'aligned.apk')],check=True)
(work/'unsigned.apk').unlink()
subprocess.run([str(bt/'apksigner.bat'),'sign','--ks','build/vr/debug.keystore','--ks-pass','pass:android','--out',str(work/'signed.apk'),str(work/'aligned.apk')],env=env,check=True)
subprocess.run([str(bt/'apksigner.bat'),'verify','--verbose',str(work/'signed.apk')],env=env,check=True)
import shutil,hashlib,json
final=Path('build/myth-standalone-185-external.apk');shutil.copyfile(work/'signed.apk',final)
report={'apk':str(final),'sha256':hashlib.sha256(final.read_bytes()).hexdigest(),'signature_verified':True,'external_only':True,'game_directory':'/storage/emulated/0/Android/data/org.mythvr.standalone/files/Game','runtime_seed':'files/runtime-seed.tar','required_external_folders':['Game','LevelPacks','SharedAssets'],'optional_external_folders':['Skyboxes'],'fresh_device_verified':False}
Path('research/reports/external-setup-apk.json').write_text(json.dumps(report,indent=2),encoding='utf8');print(json.dumps(report))

import zipfile
with zipfile.ZipFile(final) as z:
 forbidden=[n for n in z.namelist() if n.startswith('assets/engine185/') or (n.startswith('assets/levels/') and n!='assets/levels/catalog.tsv') or n in remove]
 if forbidden:raise ValueError('Original assets remain: '+repr(forbidden))
 assert z.getinfo('assets/levels/catalog.tsv').file_size==0
print('External-only archive audit passed')
