@echo off
setlocal
:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

echo [INFO] Building SushiTrack Docker image...
docker build -t sushitrack .

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Image 'sushitrack' built successfully.
) else (
    echo [ERROR] Docker build failed.
)
pause
