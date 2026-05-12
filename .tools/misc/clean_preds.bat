@echo off
setlocal enabledelayedexpansion

:: Get project root (two levels up from .tools/project)
set "ROOT_DIR=%~dp0..\.."
set "TEST_DATA_DIR=%ROOT_DIR%\test\data"

echo [INFO] Cleaning prediction directories in test/data...

set "FOUND=0"
for /d %%i in ("%TEST_DATA_DIR%\*") do (
    if exist "%%i\pred" (
        echo [DELETE] Removing: %%i\pred
        rd /s /q "%%i\pred"
        set "FOUND=1"
    )
)

if "!FOUND!"=="0" (
    echo [INFO] No 'pred' directories found to clean.
) else (
    echo [SUCCESS] All prediction directories have been cleaned.
)
