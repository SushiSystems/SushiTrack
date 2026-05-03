<#
.SYNOPSIS
    Automates the MOT Challenge evaluation comparison across multiple trackers.

.DESCRIPTION
    This script runs the Python evaluation pipeline (run_eval.py) to compare
    the tracking performance of SushiTrack against ByteTrack, BOT-SORT, and OC-SORT.
    It expects the prediction files to be available in the corresponding dataset directories.

.PARAMETER DataDir
    Optional. The root directory containing the datasets and predictions.
    If not specified, the script relies on the default resolution in run_eval.py.

.PARAMETER Split
    Optional. The dataset split to evaluate (e.g., train, test). Default is 'train'.

.PARAMETER ExportDir
    Optional. Directory to export the evaluation results (JSON/CSV).

.EXAMPLE
    .\compare_trackers.ps1
    Runs the default comparison on all available sequences.

.EXAMPLE
    .\compare_trackers.ps1 -Split "test" -ExportDir "results_out"
    Runs the comparison on the test split and exports results.

.AUTHOR
    Mustafa Garip
.DATE
    28.04.2026
#>

param (
    [string]$DataDir = "",
    [string]$Split = "train",
    [string]$ExportDir = ""
)

$ErrorActionPreference = "Stop"

# Determine the directory of this script to locate run_eval.py
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$PythonEvalScript = Join-Path $ScriptDir "python\run_eval.py"

if (-Not (Test-Path $PythonEvalScript)) {
    Write-Error "Could not find evaluation script at: $PythonEvalScript"
    exit 1
}

# Construct the base command
$Command = @("python", $PythonEvalScript, "--compare", "--trackers", "sushitrack", "bytetrack", "botsort", "ocsort", "--split", $Split)

# Append optional arguments
if ($DataDir) {
    $Command += "--data-dir"
    $Command += $DataDir
}

if ($ExportDir) {
    $Command += "--export"
    $Command += $ExportDir
}

Write-Host "[INFO] Starting tracking comparison evaluation..." -ForegroundColor Cyan
Write-Host "[INFO] Trackers: sushitrack, bytetrack, botsort, ocsort" -ForegroundColor Cyan
Write-Host "[INFO] Command: $($Command -join ' ')" -ForegroundColor DarkGray

# Execute the command
& $Command
$ExitCode = $LASTEXITCODE

if ($ExitCode -eq 0) {
    Write-Host "[SUCCESS] Comparison evaluation completed successfully." -ForegroundColor Green
} else {
    Write-Host "[ERROR] Comparison evaluation failed with exit code $ExitCode." -ForegroundColor Red
}

exit $ExitCode
