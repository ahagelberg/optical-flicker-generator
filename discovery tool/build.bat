@echo off
setlocal
cd /d "%~dp0"
python -m pip install -r requirements-build.txt
python -m PyInstaller --noconfirm --clean flicker_discovery.spec
if errorlevel 1 exit /b 1
echo.
echo Built: dist\FlickerDiscovery.exe
