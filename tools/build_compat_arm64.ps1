$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$toolchain = Join-Path $PSScriptRoot 'runtime/android-ndk-r27d/toolchains/llvm/prebuilt/windows-x86_64/bin'
$outputDir = Join-Path $projectRoot 'build/compat-arm64'
New-Item -ItemType Directory -Force $outputDir | Out-Null
$compiler = Join-Path $toolchain 'clang.exe'
$output = Join-Path $outputDir 'libmyth181_compat.so'
& $compiler '--target=aarch64-linux-android29' '-O2' '-std=c11' '-Wall' '-Wextra' '-Werror' '-fPIC' '-ffreestanding' '-fno-stack-protector' '-nostdlib' '-shared' '-Wl,--build-id=none' '-Wl,--no-undefined' '-Wl,-soname,libmyth181_compat.so' (Join-Path $projectRoot 'src/compat/primitives.c') (Join-Path $projectRoot 'src/compat/mesh.c') '-o' $output
if ($LASTEXITCODE -ne 0) { throw 'ARM64 compatibility kernel compilation failed' }
& (Join-Path $toolchain 'llvm-readelf.exe') '-h' '-d' '-r' $output | Set-Content -Encoding utf8 (Join-Path $outputDir 'elf-inspection.txt')
if ($LASTEXITCODE -ne 0) { throw 'ELF inspection failed' }
$binaryHash = (Get-FileHash $output -Algorithm SHA256).Hash.ToLowerInvariant()
$sourceHashes = @{}
foreach ($relative in @('src/compat/primitives.c', 'src/compat/primitives.h', 'src/compat/mesh.c', 'src/compat/mesh.h', 'src/compat/mesh_layout.inc')) {
    $sourceHashes[$relative] = (Get-FileHash (Join-Path $projectRoot $relative) -Algorithm SHA256).Hash.ToLowerInvariant()
}
$manifest = @{ ndk = '27.3.13750724'; target = 'aarch64-linux-android29'; optimization = 'O2'; freestanding = $true; binary_sha256 = $binaryHash; sources = $sourceHashes }
$manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $outputDir 'build-manifest.json')
Write-Output "ARM64 library SHA256: $binaryHash"
