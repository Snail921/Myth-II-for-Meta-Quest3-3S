$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$bin='tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin'
$exports=@{kernel32=@('CreateFileA@28','ReadFile@20','WriteFile@20','CloseHandle@4','Sleep@4','ExitProcess@4','GetLastError@0','LoadLibraryA@4','GetProcAddress@8','VirtualProtect@16','FlushInstructionCache@12','SetLastError@4','OpenProcess@12','WaitForSingleObject@8');user32=@('EnumWindows@8','GetWindowTextA@12','GetWindowThreadProcessId@8','SetWindowsHookExA@16','UnhookWindowsHookEx@4','CallNextHookEx@16','IsWindow@4','SendMessageTimeoutA@28','GetWindowLongA@8','SetWindowLongA@12','GetClientRect@8','ClientToScreen@8','SetWindowPos@28','CreateWindowExA@48','DestroyWindow@4')}
foreach($dll in $exports.Keys){
 $lines=@("LIBRARY $dll.dll",'EXPORTS')+$exports[$dll]
 $lines | Set-Content "build/input-x86/$dll.def" -Encoding ascii
 & "$bin/llvm-dlltool.exe" -m i386 -k -d "build/input-x86/$dll.def" -l "build/input-x86/$dll.lib"
 if($LASTEXITCODE){throw 'import library'}
}
foreach($name in @('game_hook','install_hook')){
 & "$bin/clang.exe" --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-stack-protector -fno-builtin -c "src/input/$name.c" -o "build/input-x86/$name.obj"
 if($LASTEXITCODE){throw 'C compile'}
}
& "$bin/clang.exe" --target=i686-pc-windows-msvc -c src/input/game_calls.S -o build/input-x86/game_calls.obj
if($LASTEXITCODE){throw 'assembly'}
& "$bin/ld.lld.exe" -flavor link /dll /machine:x86 /entry:DllMain /nodefaultlib /safeseh:no /export:MythInputHook=_MythInputHook@12 /out:build/input-x86/mythinput.dll build/input-x86/game_hook.obj build/input-x86/game_calls.obj build/input-x86/kernel32.lib build/input-x86/user32.lib
if($LASTEXITCODE){throw 'DLL link'}
& "$bin/ld.lld.exe" -flavor link /machine:x86 /subsystem:windows /entry:entry /nodefaultlib /safeseh:no /out:build/input-x86/mythinput.exe build/input-x86/install_hook.obj build/input-x86/kernel32.lib build/input-x86/user32.lib
if($LASTEXITCODE){throw 'EXE link'}

foreach($name in @('borderless','test_borderless')){
 $source=if($name -eq 'borderless'){'src/input/borderless.c'}else{'tools/test_borderless.c'}
 & "$bin/clang.exe" --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-stack-protector -fno-builtin -c $source -o "build/input-x86/$name.obj"
 if($LASTEXITCODE){throw 'borderless compile'}
 & "$bin/ld.lld.exe" -flavor link /machine:x86 /subsystem:windows /entry:entry /nodefaultlib /safeseh:no "/out:build/input-x86/$name.exe" "build/input-x86/$name.obj" build/input-x86/kernel32.lib build/input-x86/user32.lib
 if($LASTEXITCODE){throw 'borderless link'}
}
