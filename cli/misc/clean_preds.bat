@echo off
setlocal enabledelayedexpansion

:: Get project root (three levels up from cli/misc)
set "TOOLS_DIR=%~dp0.."
set "ROOT_DIR=%~dp0..\.."

:: Relocated data roots are configured in cli\config.json; `st paths` prints the
:: resolved value, which can be passed as the first argument here.
set "TEST_DATA_DIR=%~1"
if not defined TEST_DATA_DIR set "TEST_DATA_DIR=%ROOT_DIR%\tests\regression\evaluator\data"

echo [INFO] Cleaning prediction directories in %TEST_DATA_DIR%...

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
