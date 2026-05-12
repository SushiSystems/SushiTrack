@echo off
setlocal enabledelayedexpansion

:: Project root is two levels up from this script.
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

set BUILD_TYPE=Release
set SOURCE_DIR=.
:: Parse paths.env
for /f "usebackq eol=# delims== tokens=1,2" %%A in ("%PROJECT_ROOT%\.tools\paths.env") do set "%%A=%%B"
if not defined DIR_BUILD set "DIR_BUILD=build"
set BUILD_DIR=%DIR_BUILD%
set CLEAN=0
set BUILD_UNIT=0
set BUILD_REGRESSION=0
set USE_ASAN=0

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--clean"         set CLEAN=1
if /i "%~1"=="--unit"          set BUILD_UNIT=1
if /i "%~1"=="--regression"    set BUILD_REGRESSION=1
if /i "%~1"=="--test"          (set BUILD_UNIT=1 & set BUILD_REGRESSION=1)
if /i "%~1"=="--addsanitizer"  set USE_ASAN=1
shift
goto parse_args
:end_parse

:: ----------------------------------------------------------------
:: Compose CMake options
:: ----------------------------------------------------------------
set CMAKE_ARGS=-DBUILD_UNIT_TEST=OFF -DBUILD_REGRESSION_TEST=OFF -DBUILD_MOT_TEST=OFF

if %BUILD_UNIT%==1       set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_UNIT_TEST=ON
if %BUILD_REGRESSION%==1 set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_REGRESSION_TEST=ON
if %USE_ASAN%==1         set CMAKE_ARGS=%CMAKE_ARGS% -DUSE_ASAN=ON

:: ----------------------------------------------------------------
:: Compose build targets
:: ----------------------------------------------------------------
set TARGET_ARGS=--target sushitrack
if %BUILD_UNIT%==1       set TARGET_ARGS=%TARGET_ARGS% --target unit_test
if %BUILD_REGRESSION%==1 set TARGET_ARGS=%TARGET_ARGS% --target regression_test --target tracker_bridge

:: ----------------------------------------------------------------
:: Clean
:: ----------------------------------------------------------------
if %CLEAN%==1 (
    echo [INFO] Removing build directory: %BUILD_DIR%
    if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ----------------------------------------------------------------
:: Configure
:: ----------------------------------------------------------------
echo [INFO] Configuring %SOURCE_DIR% ...
cmake -S %SOURCE_DIR% -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %CMAKE_ARGS%
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: ----------------------------------------------------------------
:: Build
:: ----------------------------------------------------------------
echo [INFO] Building targets: %TARGET_ARGS%
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% %TARGET_ARGS%
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [SUCCESS] Build process completed.
