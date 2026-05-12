@echo off
setlocal enabledelayedexpansion

:: Get project root (three levels up from .tools/misc)
set "TOOLS_DIR=%~dp0.."
set "ROOT_DIR=%~dp0..\.."

:: Parse paths.env
for /f "usebackq eol=# delims== tokens=1,2" %%A in ("%TOOLS_DIR%\paths.env") do set "%%A=%%B"
if not defined DIR_EVALUATOR set "DIR_EVALUATOR=tests\regression\evaluator"

set "TEST_DATA_DIR=%ROOT_DIR%\%DIR_EVALUATOR%\data"

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
