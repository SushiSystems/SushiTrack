@echo off
REM `ds` entry point for cmd/PowerShell. %~dp0 keeps it working from any
REM directory; exit /b propagates the CLI's exit code to the caller.
python "%~dp0ds.py" %*
exit /b %ERRORLEVEL%
