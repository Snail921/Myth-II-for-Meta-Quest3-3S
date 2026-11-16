python (Join-Path $PSScriptRoot "prepare_water_normal.py")
if($LASTEXITCODE){throw "water normal asset"}
python (Join-Path $PSScriptRoot "prepare_water_light.py")
if($LASTEXITCODE){throw "water light assets"}
python (Join-Path $PSScriptRoot "prepare_fog_assets.py")
if($LASTEXITCODE){throw "fog assets"}
python (Join-Path $PSScriptRoot "prepare_info_font.py")
if($LASTEXITCODE){throw "info font"}
python (Join-Path $PSScriptRoot 'prepare_reflection_assets.py')
if($LASTEXITCODE){throw 'reflection assets'}
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location $root
$ndk=Join-Path $root 'tools/runtime/android-ndk-r27d'
$bin=Join-Path $ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin'
$glue=Join-Path $ndk 'sources/android/native_app_glue'
$sdk='C:/Users/gen/AppData/Local/Android/Sdk'
$bt=Join-Path $sdk 'build-tools/30.0.2'
$jdk=Join-Path $root 'tools/runtime/jdk-21.0.13+9/bin'
$xr=Join-Path $root 'tools/runtime/openxr/loader'
New-Item -ItemType Directory -Force build/vr/lib/arm64-v8a | Out-Null
python tools/prepare_ground_marker.py
if($LASTEXITCODE){throw 'ground marker asset'}
& "$bin/clang.exe" --target=aarch64-linux-android29 -O2 -fPIC -c "$glue/android_native_app_glue.c" -o build/vr/glue.o
if($LASTEXITCODE){throw 'glue compile'}
& "$bin/clang++.exe" --target=aarch64-linux-android29 -O2 -std=c++17 -fPIC -shared -static-libstdc++ -Wall -Wextra -Wno-missing-field-initializers -I $glue -I "$xr/prefab/modules/headers/include" src/vr/main.cpp build/vr/glue.o "$xr/jni/arm64-v8a/libopenxr_loader.so" -landroid -llog -lEGL -lGLESv3 -o build/vr/lib/arm64-v8a/libmythvr.so
if($LASTEXITCODE){throw 'renderer compile'}
Copy-Item "$xr/jni/arm64-v8a/libopenxr_loader.so" build/vr/lib/arm64-v8a -Force
& "$bt/aapt.exe" package -f -M src/vr/AndroidManifest.xml -I "$sdk/platforms/android-29/android.jar" -A build/vr/assets -F build/vr/base.apk
if($LASTEXITCODE){throw 'aapt package'}
@'
from pathlib import Path
import zipfile
with zipfile.ZipFile('build/vr/base.apk','a',zipfile.ZIP_DEFLATED) as z:
 for p in Path('build/vr/lib').rglob('*.so'):z.write(p,p.relative_to('build/vr').as_posix())
'@ | python -
& "$bt/zipalign.exe" -f 4 build/vr/base.apk build/vr/aligned.apk
if($LASTEXITCODE){throw 'zipalign'}
if(!(Test-Path build/vr/debug.keystore)){
 & "$jdk/keytool.exe" -genkeypair -keystore build/vr/debug.keystore -storepass android -keypass android -alias androiddebugkey -dname 'CN=Myth VR Local Development' -keyalg RSA -keysize 2048 -validity 3650
 if($LASTEXITCODE){throw 'keytool'}
}
$env:JAVA_HOME=Split-Path $jdk -Parent
& "$bt/apksigner.bat" sign --ks build/vr/debug.keystore --ks-pass pass:android --out build/vr/myth-vr.apk build/vr/aligned.apk
if($LASTEXITCODE){throw 'sign'}
& "$bt/apksigner.bat" verify build/vr/myth-vr.apk
if($LASTEXITCODE){throw 'signature verify'}
Get-FileHash build/vr/myth-vr.apk
