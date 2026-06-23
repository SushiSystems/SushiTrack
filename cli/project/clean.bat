@echo off
setlocal
:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

echo [INFO] Cleaning project build artifacts...

:: Parse paths.env
for /f "usebackq eol=# delims== tokens=1,2" %%A in ("%PROJECT_ROOT%\cli\paths.env") do set "%%A=%%B"
if not defined DIR_BUILD set "DIR_BUILD=build"

if exist "%DIR_BUILD%" (
    echo [INFO] Removing %DIR_BUILD%/
    rd /s /q "%DIR_BUILD%"
)

echo [SUCCESS] Clean completed.
