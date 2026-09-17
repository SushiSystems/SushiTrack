# Registers `st` and `sushitrack` for the current PowerShell session. Dot-source it so the
# function survives the script: . .\cli\install.ps1
$CliDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

$env:PATH = "$CliDir;" + $env:PATH

# GetNewClosure() bakes the current $CliDir value into the function so it keeps
# working even when this script is run directly (not dot-sourced): without it,
# the global function looks up $CliDir at call time, finds nothing once this
# script's scope is gone, and silently runs "python \st.py" instead.
${function:global:st} = { python "$CliDir\st.py" @args }.GetNewClosure()
${function:global:sushitrack} = { python "$CliDir\sushitrack.py" @args }.GetNewClosure()

Write-Host "SushiTrack CLI registered for this session." -ForegroundColor Green
Write-Host "st build | st test | st status | st doctor | st --help" -ForegroundColor Cyan
Write-Host ""
Write-Host "To make it permanent, add this to your `$PROFILE:" -ForegroundColor Yellow
Write-Host "function st { python `"$CliDir\st.py`" `$args }"
Write-Host "function sushitrack { python `"$CliDir\sushitrack.py`" `$args }"
