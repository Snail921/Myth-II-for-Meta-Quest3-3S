param([switch]$JavaOnly)
$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$java='tools/runtime/jdk-21.0.13+9/bin'
$bt='C:/Users/gen/AppData/Local/Android/Sdk/build-tools/30.0.2'
$env:JAVA_HOME=(Resolve-Path 'tools/runtime/jdk-21.0.13+9').Path
New-Item -ItemType Directory -Force build/standalone-java | Out-Null
if(!(Test-Path build/android-29.jar)){Copy-Item C:/Users/gen/AppData/Local/Android/Sdk/platforms/android-29/android.jar build/android-29.jar}
if(!(Test-Path build/standalone-runtime)){
 & "$java/java.exe" -jar tools/downloads/apktool_2.11.1.jar d tools/downloads/Winlator_11.2.apk -o build/standalone-runtime -p build/apktool-framework
 if($LASTEXITCODE){throw 'APK decode'}
}
if(!$JavaOnly){
 python tools/prepare_standalone_apk.py
 if($LASTEXITCODE){throw 'runtime preparation'}
}
& "$java/javac.exe" -encoding UTF-8 --release 8 -Xlint:-options -classpath build/android-29.jar -d build/standalone-java src/standalone/StandaloneActivity.java src/standalone/StandaloneBridge.java src/standalone/PanelKeyboard.java src/standalone/StartupDisc.java src/standalone/OriginalDialogs.java
if($LASTEXITCODE){throw 'launcher compile'}
& "$java/jar.exe" cf build/standalone-java.jar -C build/standalone-java org
if($LASTEXITCODE){throw 'launcher archive'}
& "$bt/d8.bat" --lib build/android-29.jar --min-api 29 --output build/standalone-java build/standalone-java.jar
if($LASTEXITCODE){throw 'launcher dex'}
if(!$JavaOnly){
& "$java/java.exe" -jar tools/downloads/apktool_2.11.1.jar b build/standalone-runtime -p build/apktool-framework -o build/myth-standalone-unsigned.apk
if($LASTEXITCODE){throw 'runtime APK build'}
}
@'
import zipfile
with zipfile.ZipFile('build/myth-standalone-unsigned.apk') as src,zipfile.ZipFile('build/myth-standalone-patched.apk','w',zipfile.ZIP_DEFLATED) as z:
 for i in src.infolist():
  if i.filename!='classes2.dex':z.writestr(i,src.read(i.filename))
 z.write('build/standalone-java/classes.dex','classes2.dex')
'@ | python -
if($LASTEXITCODE){throw 'dex packaging'}
& "$bt/zipalign.exe" -f 4 build/myth-standalone-patched.apk build/myth-standalone-aligned.apk
if($LASTEXITCODE){throw 'alignment'}
& "$bt/apksigner.bat" sign --ks build/vr/debug.keystore --ks-pass pass:android --out build/myth-standalone.apk build/myth-standalone-aligned.apk
if($LASTEXITCODE){throw 'signature'}
& "$bt/apksigner.bat" verify build/myth-standalone.apk
if($LASTEXITCODE){throw 'signature verification'}
Get-FileHash build/myth-standalone.apk
