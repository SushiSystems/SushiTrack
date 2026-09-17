@echo off
REM `st` entry point for cmd/PowerShell. %~dp0 keeps it working from any
REM directory; exit /b propagates the CLI's exit code to the caller.
python "%~dp0st.py" %*
exit /b %ERRORLEVEL%
