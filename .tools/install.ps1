$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$ToolsDir = $ScriptDir
$ProjectRoot = Split-Path -Parent $ToolsDir

# Function to add to PATH for current session
$env:PATH = "$ToolsDir;" + $env:PATH

# Create aliases for the current session
function ds { python "$ToolsDir\sushitrack.py" @args }
function sushitrack { python "$ToolsDir\sushitrack.py" @args }

Write-Host "SushiTrack Tools installed in current session." -ForegroundColor Green
Write-Host "Commands available: ds, sushitrack" -ForegroundColor Cyan
Write-Host ""
Write-Host "To make this permanent, add the following to your `$PROFILE:" -ForegroundColor Yellow
Write-Host "function ds { python `"$ToolsDir\sushitrack.py`" `$args }"
Write-Host "function sushitrack { python `"$ToolsDir\sushitrack.py`" `$args }"
