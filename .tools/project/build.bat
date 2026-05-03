@echo off
setlocal enabledelayedexpansion

:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

set BUILD_TYPE=Release
set SOURCE_DIR=.
set BUILD_DIR=build
set CLEAN=0
set TEST=0
set EXTRA_ARGS=

:parse_args
if "%~1"=="" goto end_parse
if "%~1"=="--clean" set CLEAN=1
if "%~1"=="--test"  set TEST=1
shift
goto parse_args
:end_parse

set TARGET_ARG=
if %TEST%==1 (
    set EXTRA_ARGS=-DBUILD_MOT_TEST=ON
    set TARGET_ARG=--target mot_test
) else (
    set EXTRA_ARGS=-DBUILD_MOT_TEST=OFF
    set TARGET_ARG=--target sushitrack
)

if %CLEAN%==1 (
    echo [INFO] Removing build directory: %BUILD_DIR%
    if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [INFO] Configuring %SOURCE_DIR% ...
cmake -S %SOURCE_DIR% -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %EXTRA_ARGS%

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

echo [INFO] Compiling SushiTrack ...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% %TARGET_ARG%

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo [SUCCESS] Build process completed.