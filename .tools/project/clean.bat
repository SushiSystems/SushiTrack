@echo off
setlocal
:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

echo [INFO] Cleaning project build artifacts...

if exist "build" (
    echo [INFO] Removing build/
    rd /s /q "build"
)

if exist "test\build" (
    echo [INFO] Removing test/build/
    rd /s /q "test\build"
)

echo [SUCCESS] Clean completed.
