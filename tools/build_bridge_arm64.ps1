$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$clang = Join-Path $root 'tools/runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin/clang.exe'
$out = Join-Path $root 'build/bridge-arm64'
New-Item -ItemType Directory -Force -Path $out | Out-Null
foreach ($name in @('capture', 'consume')) {
    & $clang --target=aarch64-linux-android29 -O2 -std=c11 -Wall -Wextra -Werror (Join-Path $root "src/bridge/$name.c") -o (Join-Path $out "myth-bridge-$name")
    if ($LASTEXITCODE -ne 0) { throw "Bridge build failed: $name" }
    Get-FileHash (Join-Path $out "myth-bridge-$name") -Algorithm SHA256
}
