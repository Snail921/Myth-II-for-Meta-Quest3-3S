$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$bin='tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin'
$out='build/integration-185'
& "$bin/clang.exe" --target=aarch64-linux-android29 -O2 -std=c11 -Wall -Wextra -Werror -Wno-unused-function "$out/capture.c" -lm -o "$out/libmyth_capture.so"
if($LASTEXITCODE){throw 'capture'}
foreach($n in @('game_hook','install_hook','movement')){
 & "$bin/clang.exe" --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-stack-protector -fno-builtin -Wall -Werror -Wno-unused-function -c "$out/$n.c" -o "$out/$n.obj"
 if($LASTEXITCODE){throw "C $n"}
}
& "$bin/clang.exe" --target=i686-pc-windows-msvc -c "$out/game_calls.S" -o "$out/game_calls.obj"
if($LASTEXITCODE){throw 'asm'}
& "$bin/ld.lld.exe" -flavor link /dll /machine:x86 /entry:DllMain /nodefaultlib /safeseh:no /export:MythInputHook=_MythInputHook@12 "/out:$out/mythinput-v7.dll" "$out/game_hook.obj" "$out/game_calls.obj" "$out/movement.obj" build/input-x86/kernel32.lib build/input-x86/user32.lib
if($LASTEXITCODE){throw 'hook'}
& "$bin/ld.lld.exe" -flavor link /machine:x86 /subsystem:windows /entry:entry /nodefaultlib /safeseh:no "/out:$out/mythinput.exe" "$out/install_hook.obj" build/input-x86/kernel32.lib build/input-x86/user32.lib
if($LASTEXITCODE){throw 'installer'}
