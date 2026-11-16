$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$out='build/integration-185'
$java='tools/runtime/jdk-21.0.13+9/bin'
$env:JAVA_HOME=(Resolve-Path 'tools/runtime/jdk-21.0.13+9').Path
$bt='C:/Users/gen/AppData/Local/Android/Sdk/build-tools/30.0.2'
New-Item -ItemType Directory -Force "$out/classes" | Out-Null
$sources=(Get-ChildItem "$out/java/*.java").FullName
& "$java/javac.exe" -encoding UTF-8 --release 8 -classpath build/android-29.jar -d "$out/classes" $sources
if($LASTEXITCODE){throw 'javac'}
& "$java/jar.exe" cf "$out/java.jar" -C "$out/classes" org
if($LASTEXITCODE){throw 'jar'}
& "$bt/d8.bat" --lib build/android-29.jar --min-api 29 --output "$out/classes" "$out/java.jar"
if($LASTEXITCODE){throw 'dex'}
$ndk='tools/runtime/android-ndk-r27d';$bin="$ndk/toolchains/llvm/prebuilt/windows-x86_64/bin";$glue="$ndk/sources/android/native_app_glue";$xr='tools/runtime/openxr/loader'
& "$bin/clang++.exe" --target=aarch64-linux-android29 -O2 -std=c++17 -fPIC -shared -static-libstdc++ -Wall -Wextra -Wno-missing-field-initializers -I $glue -I "$xr/prefab/modules/headers/include" "$out/vr/main.cpp" build/vr/glue.o "$xr/jni/arm64-v8a/libopenxr_loader.so" -landroid -llog -lEGL -lGLESv3 -o "$out/libmythvr.so"
if($LASTEXITCODE){throw 'renderer'}
