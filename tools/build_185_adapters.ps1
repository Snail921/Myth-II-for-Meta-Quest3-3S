$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$bin='tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin'
New-Item -ItemType Directory -Force build/input-185 | Out-Null
& "$bin/clang.exe" --target=i686-pc-windows-msvc -c src/input/engine185/game_calls.S -o build/input-185/game_calls.obj
if($LASTEXITCODE){throw '471b adapter assembly failed'}
& "$bin/ld.lld.exe" -flavor link /dll /noentry /machine:x86 /nodefaultlib /safeseh:no /base:0x2000000 /export:movement185 /export:formation185 /out:build/input-185/adapters.dll build/input-185/game_calls.obj
if($LASTEXITCODE){throw '471b adapter link failed'}
