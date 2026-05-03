@echo off
setlocal
:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

echo [INFO] Running SushiTrack Docker container...
:: Using workspace path to match working examples
docker run -it --rm -v "%cd%:/workspace/sushitrack" sushitrack bash
pause
