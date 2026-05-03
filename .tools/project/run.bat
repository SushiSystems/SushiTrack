@echo off
setlocal
:: Project root is two levels up
set "PROJECT_ROOT=%~dp0..\.."
cd /d "%PROJECT_ROOT%"

python .tools\sushitrack.py project run %*
