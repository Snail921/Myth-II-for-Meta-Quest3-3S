"""Prepare an isolated runtime+VR feasibility APK from the pinned Winlator APK.
Keeps the original installed package and its data untouched. Requires Apktool decode first.
This binary integration is a development bridge to a source-built distribution.
"""
from pathlib import Path
import hashlib,re,shutil,xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[1]
import subprocess,sys
subprocess.run([sys.executable,str(root/'tools/prepare_fog_assets.py')],check=True)
subprocess.run([sys.executable,str(root/'tools/prepare_water_light.py')],check=True)
subprocess.run([sys.executable,str(root/'tools/prepare_water_normal.py')],check=True)
out=root/'build/standalone-runtime'
ns='http://schemas.android.com/apk/res/android'
ET.register_namespace('android',ns)
meta='http://schemas.meta.com/metavr-sdk'
ET.register_namespace('metavr',meta)
def a(k):return '{'+ns+'}'+k
p=out/'AndroidManifest.xml';tree=ET.parse(p);manifest=tree.getroot()
manifest.set('package','org.mythvr.standalone')
for n in list(manifest):
    if n.tag=='{'+meta+'}uses-metavr-sdk':manifest.remove(n)
ET.SubElement(manifest,'{'+meta+'}uses-metavr-sdk',{'{'+meta+'}minSdkVersion':'69','{'+meta+'}targetSdkVersion':'69'})
app=manifest.find('application');app.set(a('label'),'Myth II Standalone Test');app.set(a('debuggable'),'false');app.set(a('allowBackup'),'false');app.set(a('extractNativeLibs'),'true')
for node in app.findall('activity'):
    for filt in list(node.findall('intent-filter')):
        if any(x.get(a('name'))=='android.intent.category.LAUNCHER' for x in filt.findall('category')):node.remove(filt)
for node in app.findall('provider'):
    v=node.get(a('authorities'),'');node.set(a('authorities'),v.replace('com.winlator','org.mythvr.standalone'))
for name in ['org.mythvr.standalone.StandaloneActivity','android.app.NativeActivity']:
    existing=next((n for n in app.findall('activity') if n.get(a('name'))==name),None)
    if existing is not None:app.remove(existing)
launcher=ET.SubElement(app,'activity',{a('name'):'org.mythvr.standalone.StandaloneActivity',a('exported'):'true',a('theme'):'@android:style/Theme.Material.NoActionBar'})
f=ET.SubElement(launcher,'intent-filter');ET.SubElement(f,'action',{a('name'):'android.intent.action.MAIN'});ET.SubElement(f,'category',{a('name'):'android.intent.category.LAUNCHER'});ET.SubElement(f,'category',{a('name'):'com.oculus.intent.category.2D'})
vr=ET.SubElement(app,'activity',{a('name'):'android.app.NativeActivity',a('exported'):'false',a('launchMode'):'singleTask',a('taskAffinity'):'org.mythvr.standalone.immersive',a('resizeableActivity'):'false',a('excludeFromRecents'):'true',a('screenOrientation'):'landscape',a('theme'):'@android:style/Theme.Black.NoTitleBar.Fullscreen',a('configChanges'):'orientation|keyboardHidden|screenSize'})
ET.SubElement(vr,'meta-data',{a('name'):'android.app.lib_name',a('value'):'mythvr'})
f=ET.SubElement(vr,'intent-filter');ET.SubElement(f,'action',{a('name'):'android.intent.action.MAIN'});ET.SubElement(f,'category',{a('name'):'android.intent.category.DEFAULT'});ET.SubElement(f,'category',{a('name'):'com.oculus.intent.category.VR'})
if not any(n.get(a('name'))=='com.oculus.supportedDevices' for n in app.findall('meta-data')):ET.SubElement(app,'meta-data',{a('name'):'com.oculus.supportedDevices',a('value'):'quest3|quest3s'})
if not any(n.get(a('name'))=='android.hardware.vr.headtracking' for n in manifest.findall('uses-feature')):ET.SubElement(manifest,'uses-feature',{a('name'):'android.hardware.vr.headtracking',a('version'):'1',a('required'):'true'})
engine=next(n for n in app.findall('activity') if n.get(a('name'))=='com.winlator.XServerDisplayActivity')
if not engine.findall('intent-filter'):
    f=ET.SubElement(engine,'intent-filter');ET.SubElement(f,'action',{a('name'):'android.intent.action.MAIN'});ET.SubElement(f,'category',{a('name'):'android.intent.category.DEFAULT'});ET.SubElement(f,'category',{a('name'):'com.oculus.intent.category.2D'})
for panel in [launcher,engine]:
    panel.set(a('screenOrientation'),'landscape')
    panel.set(a('taskAffinity'),'org.mythvr.standalone.panel.large')
    for layout in list(panel.findall('layout')):panel.remove(layout)
    ET.SubElement(panel,'layout',{a('defaultWidth'):'1920dp',a('defaultHeight'):'1080dp',a('minWidth'):'640dp',a('minHeight'):'360dp'})
tree.write(p,encoding='utf-8',xml_declaration=True)

p=out/'smali/com/winlator/XServerDisplayActivity.smali';s=p.read_text()
# Only the dedicated VR variant keeps the guest running when its 2D view pauses.
s=s.replace('invoke-virtual {v0}, Lcom/winlator/xenvironment/XEnvironment;->onPause()V','nop # Standalone VR keeps its guest simulation alive')
start=s.index('.method public onCreate(Landroid/os/Bundle;)V');end=s.index('.end method',start)
body=s[start:end]
if 'StandaloneBridge;->onEngine' not in body:body=body.replace('    return-void','    invoke-static/range {p0 .. p0}, Lorg/mythvr/standalone/StandaloneBridge;->onEngine(Landroid/app/Activity;)V\n\n    return-void')
s=s[:start]+body+s[end:]
start=s.index('.method protected onDestroy()V');end=s.index('.end method',start);body=s[start:end]
if 'StandaloneBridge;->stop' not in body:body=body.replace('    return-void','    invoke-static {}, Lorg/mythvr/standalone/StandaloneBridge;->stop()V\n\n    return-void')
s=s[:start]+body+s[end:]
# Make the original menu pointer visible on Quest's 2D surface.
s=re.sub(r'(\s+)invoke-virtual \{(v\d+), (v\d+)\}, Lcom/winlator/renderer/GLRenderer;->setCursorVisible\(Z\)V',r'\1const/4 \3, 0x1\1invoke-virtual {\2, \3}, Lcom/winlator/renderer/GLRenderer;->setCursorVisible(Z)V',s)
# Observe Touch pointer holds in the original 2D panel; retain ordinary short clicks.
if '.method public dispatchTouchEvent(Landroid/view/MotionEvent;)Z' not in s:
    s+='\n.method public dispatchTouchEvent(Landroid/view/MotionEvent;)Z\n    .locals 1\n    invoke-static {p0, p1}, Lorg/mythvr/standalone/StandaloneBridge;->panelMotion(Landroid/app/Activity;Landroid/view/MotionEvent;)V\n    invoke-super {p0, p1}, Landroidx/appcompat/app/AppCompatActivity;->dispatchTouchEvent(Landroid/view/MotionEvent;)Z\n    move-result v0\n    return v0\n.end method\n'
for signature,call in [('dispatchGenericMotionEvent(Landroid/view/MotionEvent;)Z','panelMotion(Landroid/app/Activity;Landroid/view/MotionEvent;)V'),('dispatchKeyEvent(Landroid/view/KeyEvent;)Z','panelKey(Landroid/app/Activity;Landroid/view/KeyEvent;)V')]:
    start=s.index('.method public '+signature);end=s.index('.end method',start);body=s[start:end]
    if 'StandaloneBridge;->' not in body:body=body.replace('    .locals 1','    .locals 1\n    invoke-static {p0, p1}, Lorg/mythvr/standalone/StandaloneBridge;->'+call,1)
    s=s[:start]+body+s[end:]
# Consume panel menu keys before Winlator controller/guest handling.
s=s.replace('    invoke-static {p0, p1}, Lorg/mythvr/standalone/StandaloneBridge;->panelKey(Landroid/app/Activity;Landroid/view/KeyEvent;)V', '    invoke-static {p0, p1}, Lorg/mythvr/standalone/StandaloneBridge;->panelKey(Landroid/app/Activity;Landroid/view/KeyEvent;)Z\n    move-result v0\n    if-eqz v0, :myth_keyboard_continue\n    const/4 v0, 0x1\n    return v0\n    :myth_keyboard_continue')
start=s.index('.method public onPause()V');end=s.index('.end method',start);body=s[start:end]
if 'cancelPanelHold' not in body:body=body.replace('    return-void','    invoke-static {}, Lorg/mythvr/standalone/StandaloneBridge;->cancelPanelHold()V\n    return-void')
s=s[:start]+body+s[end:]
p.write_text(s)
p=out/'smali/com/winlator/core/ProcessHelper.smali' ;s=p.read_text()
s=s.replace('"/dev/null"','"/data/user/0/org.mythvr.standalone/files/guest-launch.log"')
start=s.index('.method public static exec(Ljava/lang/String;Lcom/winlator/core/EnvVars;Ljava/io/File;Lcom/winlator/core/Callback;)I');end=s.index('.end method',start);body=s[start:end]
if 'StandaloneBridge;->relocate' not in body:
    body=body.replace('    :try_start_0','    invoke-static/range {p0 .. p0}, Lorg/mythvr/standalone/StandaloneBridge;->relocate(Ljava/lang/String;)V\n\n    :try_start_0',1)
if 'Throwable;->printStackTrace' not in body:
    body=body.replace('    move-exception v1','    move-exception v1\n\n    invoke-virtual {v1}, Ljava/lang/Throwable;->printStackTrace()V',1)
s=s[:start]+body+s[end:];p.write_text(s)
# Finishing the separate immersive task is not closing the original runtime task.
p=out/'smali/com/winlator/services/ForegroundService.smali';s=p.read_text()
start=s.index('.method public onTaskRemoved(Landroid/content/Intent;)V');end=s.index('.end method',start);body=s[start:end]
if 'keepRuntimeForRemovedTask' not in body:
    anchor='    invoke-super {p0, p1}, Landroid/app/Service;->onTaskRemoved(Landroid/content/Intent;)V'
    guard='\n    invoke-static {p1}, Lorg/mythvr/standalone/StandaloneBridge;->keepRuntimeForRemovedTask(Landroid/content/Intent;)Z\n    move-result v0\n    if-eqz v0, :myth_normal_task_removal\n    return-void\n    :myth_normal_task_removal\n'
    assert anchor in body;body=body.replace(anchor,anchor+guard);s=s[:start]+body+s[end:];p.write_text(s)

assets=out/'assets';
levelpacks=root/'build/vr/assets/levels'
if not (levelpacks/'catalog.tsv').is_file():raise RuntimeError('Run tools/build_level_assets.py first')
shutil.copytree(levelpacks,assets/'levels',dirs_exist_ok=True)
payload=assets/'standalone';payload.mkdir(exist_ok=True)
shutil.copyfile(root/'Assets/Logo.png',payload/'Logo.png')
for name in ['scene.pack','models.pack','snapshot.bin','sprite-indices.pack','animated-models.pack','surface-effects.pack','particles.pack','ground-attack-marker.rgba','reflection-water.pack','info-font.rgba','info-decoration.pack','dialog-templates.pack','fog.pack','water-light.pack','water-normal.pack']:shutil.copyfile(root/'build/vr/assets'/name,assets/name)
shutil.copyfile(root/'build/input-x86/mythinput.exe',payload/'mythinput.exe')
shutil.copyfile(root/'build/input-x86/mythinput.dll',payload/'mythinput-v7.dll')
seed=root/'research/fixtures/standalone/runtime-seed.tar'
with seed.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
(payload/'seed.sha256').write_text(digest,encoding='ascii')
lib=out/'lib/arm64-v8a'
for name in ['libmythvr.so','libopenxr_loader.so']:shutil.copyfile(root/'build/vr/lib/arm64-v8a'/name,lib/name)
shutil.copyfile(root/'build/bridge-arm64/myth-bridge-capture',lib/'libmyth_capture.so')
shutil.copyfile(root/'build/bridge-arm64/win-exec',lib/'libmyth_exec.so')
print('Prepared isolated non-debuggable standalone runtime')

from prepare_startup_disc import prepare as prepare_startup_disc
prepare_startup_disc(root)

shutil.copyfile(root/'build/input-x86/borderless.exe',payload/'borderless.exe')
