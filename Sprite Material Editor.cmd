@echo off
cd /d "%~dp0"
set PYTHONUTF8=1
python tools\sprite_material_editor.py
if errorlevel 1 pause
