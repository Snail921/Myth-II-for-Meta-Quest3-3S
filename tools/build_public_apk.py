"""Build the external-data APK from upstream Winlator and project sources.

Windows toolchain layout: tools/runtime/{jdk-21.0.13+9,android-ndk-r27d,openxr/loader}.
Requires Android SDK API 29 / build-tools 30.0.2 and Apktool 2.11.1.
Never consumes the old private candidate APK or original game archives.
"""
import argparse, hashlib, json, os, shutil, struct, subprocess, sys, zipfile
from pathlib import Path
from patch_public_runtime import patch

ROOT=Path(__file__).resolve().parents[1]
UPSTREAM_SHA256='ea5c81bf0e5b90a6b9ccb8007121ab6738c8646a5a60b341ce0ac2b33f1d100a'

def run(*args, env=None):
    subprocess.run([str(a) for a in args],check=True,cwd=ROOT,env=env)

def build(args):
    os.chdir(ROOT)
    source=Path(args.winlator_apk).resolve(); seed=Path(args.runtime).resolve()
    with source.open('rb') as f:
        if hashlib.file_digest(f,'sha256').hexdigest()!=UPSTREAM_SHA256:
            raise ValueError('Wrong upstream Winlator APK')
    with seed.open('rb') as f: seed_hash=hashlib.file_digest(f,'sha256').hexdigest()
    work=Path(args.work).resolve()
    if work.exists() and not args.resume:raise ValueError('Use a new build directory')
    work.mkdir(parents=True,exist_ok=True)
    env=dict(os.environ,PYTHONUTF8='1',PYTHONIOENCODING='utf-8')
    jdk=ROOT/'tools/runtime/jdk-21.0.13+9'
    ndk=ROOT/'tools/runtime/android-ndk-r27d'
    sdk=Path(args.sdk).resolve(); bt=sdk/'build-tools/30.0.2'
    env.update(JAVA_HOME=str(jdk),ANDROID_HOME=str(sdk))
    java=jdk/'bin/java.exe'; bin=ndk/'toolchains/llvm/prebuilt/windows-x86_64/bin'
    xr=ROOT/'tools/runtime/openxr/loader'
    glue=ndk/'sources/android/native_app_glue'
    (ROOT/'build/input-x86').mkdir(parents=True,exist_ok=True)
    (ROOT/'build/vr').mkdir(parents=True,exist_ok=True)
    shutil.copyfile(sdk/'platforms/android-29/android.jar',ROOT/'build/android-29.jar')
    generated=ROOT/'build/integration-185'
    decoded=work/'decoded'
    if not args.resume:
        print('Generating 1.8.5 integration sources',flush=True)
        run(sys.executable,'tools/prepare_185_full.py',env=env)
        generated=ROOT/'build/integration-185'
        # Test package uses separate external storage; Java class names remain unchanged.
        if args.package_id!='org.mythvr.standalone':
            for f in (generated/'java').glob('*.java'):
                s=f.read_text(encoding='utf8').replace('org.mythvr.standalone/files',args.package_id+'/files')
                f.write_text(s,encoding='utf8')
        print('Building Windows input helpers and ARM64 renderer',flush=True)
        run('powershell','-NoProfile','-ExecutionPolicy','Bypass','-File','tools/build_game_input.ps1',env=env)
        run('powershell','-NoProfile','-ExecutionPolicy','Bypass','-File','tools/build_185_integration_native.ps1',env=env)
        run(bin/'clang.exe','--target=aarch64-linux-android29','-O2','-fPIC','-c',glue/'android_native_app_glue.c','-o',ROOT/'build/vr/glue.o',env=env)
        run('powershell','-NoProfile','-ExecutionPolicy','Bypass','-File','tools/build_185_integration_app.ps1',env=env)
        run(bin/'clang.exe','--target=aarch64-linux-android29','-O2','src/input/win_exec.c','-o',work/'libmyth_exec.so',env=env)
        print('Preparing upstream APK',flush=True)
        decoded=work/'decoded'
        run(java,'-jar',args.apktool,'d',source,'-o',decoded,'-p',work/'framework',env=env)
    patch(decoded)
    import xml.etree.ElementTree as ET
    ns='{http://schemas.android.com/apk/res/android}'
    manifest=decoded/'AndroidManifest.xml';tree=ET.parse(manifest);m=tree.getroot()
    m.set('package',args.package_id)
    # Keep the application version independent of upstream Winlator.
    import re
    metadata=decoded/'apktool.yml'
    y=metadata.read_text(encoding='utf8')
    y=re.sub(r'(?m)^  versionCode:.*$', '  versionCode: 41', y)
    y=re.sub(r'(?m)^  versionName:.*$', '  versionName: 0.5.0-pbr-like.3', y)
    metadata.write_text(y,encoding='utf8')
    app=m.find('application');app.set(ns+'debuggable','true' if args.package_id!='org.mythvr.standalone' else 'false');app.set(ns+'label','Myth II for Meta Quest 3 / 3S' if args.package_id=='org.mythvr.standalone' else 'Myth II VR Runtime Test')
    for node in app.iter():
        for key,value in list(node.attrib.items()):
            if key in (ns+'authorities',ns+'taskAffinity'):
                node.set(key,value.replace('org.mythvr.standalone',args.package_id))
    tree.write(manifest,encoding='utf-8',xml_declaration=True)
    if args.package_id!='org.mythvr.standalone':
        path=decoded/'smali/com/winlator/core/ProcessHelper.smali'
        path.write_text(path.read_text().replace('/org.mythvr.standalone/','/'+args.package_id+'/'))
    assets=decoded/'assets'
    # Keep only Wine builtins: native Windows component installers are not shipped.
    for f in (assets/'wincomponents').glob('*.tzst'):f.unlink()
    # The upstream template carries optional fonts; the runtime uses Wine fonts.
    import io, tarfile, zstandard
    template=assets/'container_pattern.tzst'
    with zstandard.ZstdDecompressor().stream_reader(io.BytesIO(template.read_bytes())) as stream:
        archive=io.BytesIO()
        with tarfile.open(fileobj=stream,mode='r|') as inp,tarfile.open(fileobj=archive,mode='w') as out:
            for member in inp:
                if '/windows/Fonts/' in member.name.replace('\\','/'):
                    continue
                out.addfile(member,inp.extractfile(member) if member.isfile() else None)
    template.write_bytes(zstandard.ZstdCompressor(level=10).compress(archive.getvalue()))
    standalone=assets/'standalone';standalone.mkdir(exist_ok=True)
    for name,src in [('mythinput.exe',generated/'mythinput.exe'),('mythinput-v7.dll',generated/'mythinput-v7.dll'),('borderless.exe',ROOT/'build/input-x86/borderless.exe')]:
        shutil.copyfile(src,standalone/name)
    (standalone/'seed.sha256').write_text(seed_hash,encoding='ascii')
    (assets/'levels').mkdir(exist_ok=True);(assets/'levels/catalog.tsv').write_text('',encoding='ascii')
    from PIL import Image
    with Image.open(ROOT/'Assets/GroundAttackMarker.png') as im:
        im=im.convert('RGBA');(assets/'ground-attack-marker.rgba').write_bytes(struct.pack('<II',*im.size)+im.tobytes())
    with Image.open(ROOT/'Assets/Water/WaterNormal.png') as im:
        im=im.convert('RGB');(assets/'water-normal.pack').write_bytes(b'MYTHWN01'+struct.pack('<II',*im.size)+bytes(v for pixel in im.getdata() for v in pixel[:2]))
    licenses=assets/'licenses';licenses.mkdir(exist_ok=True)
    license_file=ROOT/'LICENSES/Winlator-LGPL-2.1.txt'
    if not license_file.is_file():license_file=ROOT/'research/references/winlator-app/LICENSE'
    shutil.copyfile(license_file,licenses/'Winlator-LGPL-2.1.txt')
    for f in (ROOT/'src/vr/licenses').glob('*.txt'):shutil.copyfile(f,licenses/f.name)
    lib=decoded/'lib/arm64-v8a'
    for name,src in [('libmyth_exec.so',work/'libmyth_exec.so'),('libmyth_capture.so',generated/'libmyth_capture.so'),('libmythvr.so',generated/'libmythvr.so'),('libopenxr_loader.so',xr/'jni/arm64-v8a/libopenxr_loader.so')]:
        shutil.copyfile(src,lib/name)
    run(java,'-jar',args.apktool,'b',decoded,'-p',work/'framework','-o',work/'base.apk',env=env)
    run(sys.executable,'tools/patch_apk_fast.py',work/'base.apk',work/'unsigned.apk','classes2.dex='+str(generated/'classes/classes.dex'),env=env)
    run(bt/'zipalign.exe','-f','4',work/'unsigned.apk',work/'aligned.apk',env=env)
    output=Path(args.out).resolve();output.parent.mkdir(parents=True,exist_ok=True)
    run(bt/'apksigner.bat','sign','--ks',Path(args.keystore).resolve(),'--ks-pass','env:MYTH_SIGNING_PASSWORD','--out',output,work/'aligned.apk',env=env)
    run(bt/'apksigner.bat','verify','--verbose',output,env=env)
    with zipfile.ZipFile(output) as z:
        forbidden=[n for n in z.namelist() if n.startswith('assets/engine185/') or n.startswith('assets/wincomponents/') and n.endswith('.tzst') or n.startswith('assets/levels/') and n!='assets/levels/catalog.tsv']
        if forbidden:raise ValueError('Unexpected payload: '+repr(forbidden))
    with output.open('rb') as f:apk_hash=hashlib.file_digest(f,'sha256').hexdigest()
    output.with_suffix('.json').write_text(json.dumps(dict(apk_sha256=apk_hash,runtime_sha256=seed_hash,upstream_sha256=UPSTREAM_SHA256,package_id=args.package_id,private_candidate_used=False),indent=2)+'\n',encoding='utf8')
    print('Built '+str(output),flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--winlator-apk',required=True);parser.add_argument('--runtime',required=True)
    parser.add_argument('--work',required=True);parser.add_argument('--out',required=True)
    parser.add_argument('--keystore',required=True)
    parser.add_argument('--sdk',default=os.environ.get('ANDROID_HOME',str(Path(os.environ['LOCALAPPDATA'])/'Android/Sdk')))
    parser.add_argument('--apktool',default=str(ROOT/'tools/downloads/apktool_2.11.1.jar'))
    parser.add_argument('--resume',action='store_true',help=argparse.SUPPRESS)
    parser.add_argument('--package-id',default='org.mythvr.standalone')
    build(parser.parse_args())

