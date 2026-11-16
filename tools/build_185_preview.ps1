$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
python tools/prepare_185_integration.py
if($LASTEXITCODE){throw 'Generate 185 preview'}
& "$PSScriptRoot/build_185_integration_native.ps1"
& "$PSScriptRoot/build_185_integration_app.ps1"
python tools/package_185_preview.py
if($LASTEXITCODE){throw 'Package 185 preview'}
