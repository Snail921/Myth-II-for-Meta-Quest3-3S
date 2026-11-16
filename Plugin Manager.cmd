@echo off
cd /d "%~dp0"
set PYTHONUTF8=1
python tools\plugin_manager.py
if errorlevel 1 pause
