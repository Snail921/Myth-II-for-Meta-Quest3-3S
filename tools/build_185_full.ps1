$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
python tools/build_hd_bindings.py
if($LASTEXITCODE){throw 'Generate HD bindings'}
python tools/prepare_185_full.py
if($LASTEXITCODE){throw 'Generate 185 candidate'}
& "$PSScriptRoot/build_185_integration_native.ps1"
& "$PSScriptRoot/build_185_integration_app.ps1"
python tools/package_185_full.py
if($LASTEXITCODE){throw 'Package 185 candidate'}
