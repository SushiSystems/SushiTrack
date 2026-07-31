# Registers `ds` for the current PowerShell session. Dot-source it so the
# function survives the script: . .\cli\install.ps1
$CliDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

$env:PATH = "$CliDir;" + $env:PATH

# GetNewClosure() bakes the current $CliDir value into the function so it keeps
# working even when this script is run directly (not dot-sourced): without it,
# the global function looks up $CliDir at call time, finds nothing once this
# script's scope is gone, and silently runs "python \ds.py" instead.
${function:global:ds} = { python "$CliDir\ds.py" @args }.GetNewClosure()

Write-Host "SushiTrack CLI registered for this session." -ForegroundColor Green
Write-Host "ds build | ds test | ds status | ds doctor | ds --help" -ForegroundColor Cyan
Write-Host ""
Write-Host "To make it permanent, add this to your `$PROFILE:" -ForegroundColor Yellow
Write-Host "function ds { python `"$CliDir\ds.py`" `$args }"
